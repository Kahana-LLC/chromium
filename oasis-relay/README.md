# Oasis Telemetry Relay

Reference implementation of the relay that sits between Oasis browsers and
telemetry sinks. The browser uploads privacy-reduced LLM paste events here;
the relay authenticates the device and fans events out to configured sinks.
Third-party credentials (Datadog API key, Splunk HEC token) live only on the
relay, never in the browser.

## Requirements

- Node.js 18+ (no external dependencies)

## Run

```bash
cp config.example.json config.json   # then edit
node server.js config.json
```

## TLS — required

The browser **refuses non-HTTPS relay URLs** (localhost is tolerated for
development only). Never launch the browser with
`--ignore-certificate-errors`; it is not needed and it undermines the
security posture of the whole pipeline.

Two supported setups:

1. **Direct TLS**: set `listen.tls.certPath` / `listen.tls.keyPath` to a
   CA-signed certificate (e.g. Let's Encrypt via certbot for
   `telemetry.example.com`).
2. **Reverse proxy**: run the relay on localhost behind nginx/Caddy/cloud
   load balancer that terminates TLS with a real certificate.

## Endpoints

| Method | Path | Auth | Purpose |
|---|---|---|---|
| POST | `/v1/enroll` | none (first contact) | Issue a device token for a `device_id`. |
| POST | `/v1/rotate` | device bearer token | Rotate the calling device's token. |
| POST | `/v1/revoke` | `X-Admin-Key` header | Revoke a device's token. The browser re-enrolls on the next upload after receiving 401. |
| POST | `/v1/events` | device bearer token | Ingest a batch of telemetry events. |
| GET | `/healthz` | none | Liveness check. |

Harden `/v1/enroll` for production by gating it on an MDM-delivered
enrollment secret or network allowlist; the reference implementation accepts
any well-formed device ID.

## Sinks

Configured under `sinks` in `config.json`; all configured sinks receive
every event (failures are logged per-sink and do not block others):

- `datadog` — Logs intake (`DD-API-KEY` header), fields: `site`, `apiKey`, `service`, `ddsource`
- `splunk_hec` — Splunk HTTP Event Collector, fields: `url`, `token`, `sourcetype`
- `webhook` — generic JSON POST for SIEM/SOAR, fields: `url`, `headers`
- `stdout` — print events (development)

## Event sanitization

The relay strips any field not in the schema allowlist and overwrites
`device_id` with the identity of the authenticated device, so clients cannot
spoof other devices or attach extra payloads. The forwarded schema matches
`chrome/browser/oasis/PRIVACY.md` in the browser tree plus a
`schema_version` field.
