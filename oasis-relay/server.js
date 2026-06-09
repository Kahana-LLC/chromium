// Oasis telemetry relay.
//
// Responsibilities:
//  - Terminate TLS with a real CA-signed certificate (or run behind a
//    reverse proxy that does). The browser refuses non-HTTPS relays, so
//    never rely on --ignore-certificate-errors.
//  - Enroll devices and issue/rotate/revoke per-device bearer tokens.
//  - Authenticate every event upload.
//  - Forward sanitized events to pluggable sinks (Datadog, Splunk HEC,
//    generic webhook, stdout). Third-party API keys live only here.
//
// Endpoints:
//   POST /v1/enroll   {device_id}            -> {token}
//   POST /v1/rotate   (Authorization bearer) -> {token}
//   POST /v1/revoke   {device_id} + X-Admin-Key header -> 204
//   POST /v1/events   (Authorization bearer) {events: [...]} -> 202
//   GET  /healthz     -> 200

import { createServer as createHttpServer } from "node:http";
import { createServer as createHttpsServer } from "node:https";
import { randomBytes, timingSafeEqual } from "node:crypto";
import { readFileSync, writeFileSync, existsSync } from "node:fs";

const SCHEMA_VERSION = 1;
const MAX_BODY_BYTES = 1024 * 1024;

// Fields allowed through to sinks. Anything else a client sends is dropped,
// so a compromised or outdated browser cannot exfiltrate extra data.
const ALLOWED_EVENT_FIELDS = new Set([
  "event_type",
  "provider",
  "domain",
  "origin",
  "content_length_bucket",
  "content_hash",
  "detection_source",
  "enforcement_mode",
  "decision",
  "classifications",
  "timestamp",
  "device_id",
]);

const configPath = process.argv[2] ?? "./config.json";
const config = JSON.parse(readFileSync(configPath, "utf8"));

// ---------------------------------------------------------------------------
// Token store: device_id -> { token, issuedAt, revoked }

const tokenStorePath = config.tokenStorePath ?? "./tokens.json";
const tokenStore = existsSync(tokenStorePath)
  ? JSON.parse(readFileSync(tokenStorePath, "utf8"))
  : {};

function persistTokens() {
  writeFileSync(tokenStorePath, JSON.stringify(tokenStore, null, 2));
}

function issueToken(deviceId) {
  const token = randomBytes(32).toString("hex");
  tokenStore[deviceId] = {
    token,
    issuedAt: new Date().toISOString(),
    revoked: false,
  };
  persistTokens();
  return token;
}

function deviceForToken(token) {
  if (!token) return null;
  for (const [deviceId, entry] of Object.entries(tokenStore)) {
    if (entry.revoked) continue;
    const a = Buffer.from(entry.token);
    const b = Buffer.from(token);
    if (a.length === b.length && timingSafeEqual(a, b)) {
      return deviceId;
    }
  }
  return null;
}

// ---------------------------------------------------------------------------
// Sinks

async function sendToDatadog(sink, events) {
  const url = `https://http-intake.logs.${sink.site ?? "datadoghq.com"}/api/v2/logs`;
  const body = events.map((event) => ({
    ddsource: sink.ddsource ?? "oasis",
    service: sink.service ?? "oasis-browser",
    message: `llm_paste ${event.provider ?? "unknown"} ${event.decision ?? ""}`,
    ...event,
  }));
  const response = await fetch(url, {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
      "DD-API-KEY": sink.apiKey,
    },
    body: JSON.stringify(body),
  });
  if (!response.ok) {
    throw new Error(`datadog sink: HTTP ${response.status}`);
  }
}

async function sendToSplunkHec(sink, events) {
  const body = events
    .map((event) =>
      JSON.stringify({
        event,
        sourcetype: sink.sourcetype ?? "oasis:llm_paste",
        time: Date.parse(event.timestamp ?? "") / 1000 || undefined,
      }),
    )
    .join("\n");
  const response = await fetch(sink.url, {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
      Authorization: `Splunk ${sink.token}`,
    },
    body,
  });
  if (!response.ok) {
    throw new Error(`splunk_hec sink: HTTP ${response.status}`);
  }
}

async function sendToWebhook(sink, events) {
  const response = await fetch(sink.url, {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
      ...(sink.headers ?? {}),
    },
    body: JSON.stringify({ schema_version: SCHEMA_VERSION, events }),
  });
  if (!response.ok) {
    throw new Error(`webhook sink: HTTP ${response.status}`);
  }
}

async function fanOut(events) {
  const results = await Promise.allSettled(
    (config.sinks ?? []).map((sink) => {
      switch (sink.type) {
        case "datadog":
          return sendToDatadog(sink, events);
        case "splunk_hec":
          return sendToSplunkHec(sink, events);
        case "webhook":
          return sendToWebhook(sink, events);
        case "stdout":
          for (const event of events) {
            console.log(JSON.stringify(event));
          }
          return Promise.resolve();
        default:
          return Promise.reject(new Error(`unknown sink type: ${sink.type}`));
      }
    }),
  );
  for (const result of results) {
    if (result.status === "rejected") {
      console.error("sink error:", result.reason?.message ?? result.reason);
    }
  }
}

// ---------------------------------------------------------------------------
// HTTP plumbing

function readBody(req) {
  return new Promise((resolve, reject) => {
    let size = 0;
    const chunks = [];
    req.on("data", (chunk) => {
      size += chunk.length;
      if (size > MAX_BODY_BYTES) {
        reject(new Error("body too large"));
        req.destroy();
        return;
      }
      chunks.push(chunk);
    });
    req.on("end", () => resolve(Buffer.concat(chunks).toString("utf8")));
    req.on("error", reject);
  });
}

function json(res, status, body) {
  res.writeHead(status, { "Content-Type": "application/json" });
  res.end(JSON.stringify(body));
}

function bearerToken(req) {
  const header = req.headers.authorization ?? "";
  return header.startsWith("Bearer ") ? header.slice(7) : null;
}

function sanitizeEvent(raw, deviceId) {
  if (typeof raw !== "object" || raw === null) return null;
  const event = {};
  for (const [key, value] of Object.entries(raw)) {
    if (ALLOWED_EVENT_FIELDS.has(key)) {
      event[key] = value;
    }
  }
  // The authenticated device, not the client payload, is authoritative.
  event.device_id = deviceId;
  event.schema_version = SCHEMA_VERSION;
  return event;
}

async function handle(req, res) {
  try {
    if (req.method === "GET" && req.url === "/healthz") {
      return json(res, 200, { ok: true });
    }

    if (req.method !== "POST") {
      return json(res, 405, { error: "method not allowed" });
    }

    if (req.url === "/v1/enroll") {
      const body = JSON.parse(await readBody(req));
      const deviceId = body?.device_id;
      if (typeof deviceId !== "string" || deviceId.length < 8) {
        return json(res, 400, { error: "invalid device_id" });
      }
      return json(res, 200, { token: issueToken(deviceId) });
    }

    if (req.url === "/v1/rotate") {
      const deviceId = deviceForToken(bearerToken(req));
      if (!deviceId) {
        return json(res, 401, { error: "invalid token" });
      }
      return json(res, 200, { token: issueToken(deviceId) });
    }

    if (req.url === "/v1/revoke") {
      const adminKey = req.headers["x-admin-key"];
      if (!adminKey || adminKey !== config.adminKey) {
        return json(res, 403, { error: "forbidden" });
      }
      const body = JSON.parse(await readBody(req));
      const entry = tokenStore[body?.device_id];
      if (!entry) {
        return json(res, 404, { error: "unknown device" });
      }
      entry.revoked = true;
      persistTokens();
      res.writeHead(204);
      return res.end();
    }

    if (req.url === "/v1/events") {
      const deviceId = deviceForToken(bearerToken(req));
      if (!deviceId) {
        return json(res, 401, { error: "invalid token" });
      }
      const body = JSON.parse(await readBody(req));
      const events = Array.isArray(body?.events)
        ? body.events.map((event) => sanitizeEvent(event, deviceId)).filter(Boolean)
        : [];
      if (events.length > 0) {
        await fanOut(events);
      }
      return json(res, 202, { accepted: events.length });
    }

    return json(res, 404, { error: "not found" });
  } catch (error) {
    console.error("request error:", error.message);
    return json(res, 400, { error: "bad request" });
  }
}

// ---------------------------------------------------------------------------
// Startup

const listen = config.listen ?? { host: "127.0.0.1", port: 8443 };
let server;
if (listen.tls?.certPath && listen.tls?.keyPath) {
  server = createHttpsServer(
    {
      cert: readFileSync(listen.tls.certPath),
      key: readFileSync(listen.tls.keyPath),
    },
    handle,
  );
} else {
  // Plain HTTP is only acceptable behind a TLS-terminating reverse proxy
  // or for localhost development.
  server = createHttpServer(handle);
}

server.listen(listen.port, listen.host, () => {
  const scheme = listen.tls ? "https" : "http";
  console.log(`oasis-relay listening on ${scheme}://${listen.host}:${listen.port}`);
});
