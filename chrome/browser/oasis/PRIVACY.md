# Oasis LLM Paste Telemetry — Privacy Posture

This document describes exactly what Oasis paste telemetry collects, what it
never collects, and which controls administrators have. It applies to the
native browser-process detection path in `chrome/browser/oasis/`.

## What is collected (per paste into a monitored LLM site)

| Field | Example | Notes |
|---|---|---|
| `event_type` | `llm_paste` | Constant. |
| `provider` | `openai` | Derived in the browser process from the committed frame URL. |
| `domain` | `chatgpt.com` | The registry pattern that matched. |
| `origin` | `https://chatgpt.com` | Scheme + host only. Never path, query or fragment. |
| `content_length_bucket` | `1k-10k` | Coarse buckets: `0-100`, `100-1k`, `1k-10k`, `10k+`. Exact lengths are never reported. |
| `content_hash` | *(absent)* | SHA-256 of pasted text. **Off by default**; only present when the `OasisTelemetryIncludeContentHash` policy is explicitly enabled. Hashes of guessable or low-entropy content can leak information — leave this off unless you have a concrete matching use case. |
| `detection_source` | `native` | Browser-process clipboard hook. |
| `enforcement_mode` | `audit` | The mode in effect (`audit`/`warn`/`block`). |
| `decision` | `audit_allow` | `audit_allow`, `warn_proceed`, `warn_cancel`, `block`, `block_sensitive`. |
| `classifications` | `["credential_assignment"]` | Category names only, from the on-device classifier. The matched text never leaves the device. |
| `timestamp` | ISO 8601 | Event time. |
| `device_id` | UUID | Random per-installation identifier; carries no user PII by itself. |

## What is never collected

- Raw pasted text, HTML, images, files or any clipboard payload.
- Full page URLs (paths, query strings, document titles).
- Keystrokes or any input outside of pastes into monitored LLM sites.
- Anything from incognito or guest sessions (the telemetry service is not
  created for them).
- Anything when `OasisTelemetryEnabled` is unset or false (default).

## Where data goes

Events are POSTed in batches over HTTPS to the enterprise-configured Oasis
relay (`OasisTelemetryRelayUrl`). The relay authenticates the per-device
token, then forwards sanitized events to the configured sinks (e.g. Datadog).
Third-party API keys live only on the relay, never in the browser. The
relay must present a CA-signed certificate; the browser refuses non-HTTPS
relay URLs (localhost excepted for development).

## Retention

The browser retains events in memory only, until the next batch upload
(default 10 seconds / 20 events). Retention beyond that is a relay/sink
concern; tenants should configure sink-level retention (e.g. Datadog log
retention) according to their own policy.

## Administrator controls

| Policy | Effect |
|---|---|
| `OasisTelemetryEnabled` | Master switch. Default off. |
| `OasisTelemetryRelayUrl` | HTTPS relay endpoint. |
| `OasisTelemetryMonitoredDomains` | Additional monitored domains. |
| `OasisTelemetryIncludeContentHash` | Opt-in content hashing. Default off. |
| `OasisPasteEnforcementMode` | `audit` (default), `warn`, `block`. |
| `OasisPasteEnforcementDomainOverrides` | Per-domain mode overrides. |
| `OasisPasteBlockSensitiveData` | Block pastes classified as sensitive. |
