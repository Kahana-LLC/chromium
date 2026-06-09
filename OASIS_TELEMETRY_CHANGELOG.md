# Oasis LLM Paste Telemetry — Change Log

**Branch:** `enterprise-chromium`
**Date:** June 9, 2026
**Scope:** Hardens the LLM paste telemetry MVP into a native, policy-managed,
enforcement-capable pipeline, per the remediation plan derived from the
security assessment (Sections 9.1–9.7).

---

## TL;DR for developers

Paste detection no longer needs the injected JavaScript listener. Detection
now happens **natively in the browser process**, on the same clipboard path
Chrome's own enterprise connectors use (`ClipboardHostImpl` →
`ChromeContentBrowserClient::IsClipboardPasteAllowedByPolicy`). Every paste —
keyboard, context menu, app menu — is evaluated **before content reaches the
page**. The feature is configured entirely through enterprise policy, is
audit-only by default, and supports warn and block modes.

```
User paste
  → ClipboardHostImpl (content layer)
  → ChromeContentBrowserClient::IsClipboardPasteAllowedByPolicy
  → oasis::OasisPasteInterceptor::EvaluatePaste        [NEW]
      ├─ not a monitored LLM site → continue normal flow
      ├─ audit  → record telemetry, continue
      ├─ warn   → dialog → proceed/cancel (decision recorded)
      └─ block  → paste denied (recorded)
  → enterprise_data_protection::PasteAllowedRequest (pre-existing flow)
```

Telemetry events go to the Oasis relay over HTTPS; the relay holds all
third-party credentials (Datadog etc.) and fans out to configured sinks.

---

## New code

### `chrome/browser/oasis/` — new module

| File | Purpose |
|---|---|
| `oasis_paste_interceptor.{h,cc}` | Entry point from the clipboard path. Resolves provider, classification, enforcement mode; emits telemetry; shows warn dialog; denies blocked pastes. |
| `llm_provider_registry.{h,cc}` | **Single source of truth** for monitored LLM destinations (replaces the duplicated C++/JS provider lists). 12 built-in providers (chatgpt.com, claude.ai, gemini.google.com, copilot.microsoft.com, perplexity.ai, …) plus policy-supplied domains. Host-suffix matching — `gemini.google.com` matches but `mail.google.com` does not. |
| `oasis_paste_policy.{h,cc}` | Resolves audit/warn/block, honoring per-domain overrides. |
| `oasis_sensitive_data_classifier.{h,cc}` | On-device RE2 classification: AWS access keys, private key blocks, JWTs, SSNs, Luhn-validated card numbers, credential assignments. Only category names ever leave the device. |
| `oasis_paste_telemetry_event.{h,cc}` | Privacy-reduced event schema + coarse length bucketing. |
| `oasis_telemetry_service.{h,cc}` + `_factory.{h,cc}` | Per-profile `KeyedService`: batching, HTTPS-only relay upload, 401-triggered re-enrollment. Not created for incognito/guest. |
| `oasis_device_identity_manager.{h,cc}` | Per-device UUID + relay-issued bearer token, 30-day rotation, revocation handling. Stored in local state. |
| `PRIVACY.md` | The privacy posture: what is collected, what never is, retention, admin controls. |
| `*_unittest.cc` (3 files) | Registry matching (incl. lookalike-host rejection), classifier detection, length bucketing. Registered in `chrome/test/BUILD.gn`. |

### `oasis-relay/` — reference relay (Node 18+, no deps)

- `POST /v1/enroll`, `/v1/rotate`, `/v1/revoke` (admin), `/v1/events`, `GET /healthz`
- Bearer-token auth per device; revoked devices get 401 and the browser
  re-enrolls automatically
- **Event sanitization**: strips any field not in the schema allowlist and
  overwrites `device_id` with the authenticated identity — clients cannot
  spoof devices or attach extra payloads
- Pluggable sinks: `datadog` (Logs intake), `splunk_hec`, `webhook`, `stdout`
- TLS: direct (cert/key in config) or behind a TLS-terminating reverse proxy.
  **Never use `--ignore-certificate-errors`** — the browser refuses non-HTTPS
  relay URLs by design (localhost excepted for development).

---

## Modified files

| File | Change |
|---|---|
| `chrome/browser/chrome_content_browser_client.cc` | `IsClipboardPasteAllowedByPolicy` now routes through `OasisPasteInterceptor::EvaluatePaste` first; the pre-existing enterprise flow runs as its continuation. |
| `chrome/common/chrome_isolated_world_ids.h` | Added `ISOLATED_WORLD_ID_OASIS_TELEMETRY` (reserved; stop reusing `ISOLATED_WORLD_ID_CHROME_INTERNAL` for any future script injection). |
| `chrome/android/java/.../ChromeIsolatedWorldIds.java` | Kept in sync per the `LINT.IfChange` contract. |
| `chrome/browser/prefs/browser_prefs.cc` | Registers Oasis profile + local-state prefs (desktop only). |
| `chrome/browser/policy/configuration_policy_handler_list_factory.cc` | Policy → pref mappings for all 9 new policies. |
| `chrome/browser/BUILD.gn`, `chrome/test/BUILD.gn` | New sources/tests wired in (desktop, `!is_android`). |
| `components/policy/resources/templates/policies.yaml` | Policy ids 1423–1431, atomic group 63 (`OasisTelemetry`). |
| `components/policy/resources/templates/policy_definitions/OasisTelemetry/` | New policy group (9 definitions + group metadata). |

---

## Enterprise policies (all new)

| Policy | Type | Default | Purpose |
|---|---|---|---|
| `OasisTelemetryEnabled` | bool | **false** | Master switch. Nothing runs unless set. |
| `OasisTelemetryRelayUrl` | string | — | HTTPS relay endpoint (non-HTTPS rejected). |
| `OasisTelemetryMonitoredDomains` | list | — | Extra domains: `"host"` or `"host=provider"`. |
| `OasisTelemetryBatchIntervalSeconds` | int | 10 | Upload cadence. |
| `OasisTelemetryBatchMaxEvents` | int | 20 | Immediate-flush threshold. |
| `OasisTelemetryIncludeContentHash` | bool | **false** | Opt-in SHA-256 of pasted text. Leave off (hash of guessable content leaks information). |
| `OasisPasteEnforcementMode` | enum | `audit` | `audit` / `warn` / `block`. |
| `OasisPasteEnforcementDomainOverrides` | dict | — | Per-domain mode, e.g. `{"chatgpt.com": "block"}`. |
| `OasisPasteBlockSensitiveData` | bool | false | Block pastes the classifier flags, regardless of mode. |

`--oasis-relay-url` remains as a development-only command-line override.
User/group targeting is done by the management server assigning different
policy values per group; the browser applies the resolved per-profile policy.

---

## Privacy posture (summary — see `chrome/browser/oasis/PRIVACY.md`)

Collected per event: provider, matched domain, **origin only** (never full
URL), **coarse length bucket** (never exact length), enforcement mode +
decision, classifier category names, timestamp, device UUID,
`detection_source: "native"`.

Never collected: raw pasted content of any kind, full URLs/titles,
keystrokes, anything from incognito/guest, anything while the policy is off.

---

## Telemetry event example

```json
{
  "event_type": "llm_paste",
  "provider": "openai",
  "domain": "chatgpt.com",
  "origin": "https://chatgpt.com",
  "content_length_bucket": "100-1k",
  "detection_source": "native",
  "enforcement_mode": "warn",
  "decision": "warn_proceed",
  "classifications": ["credential_assignment"],
  "timestamp": "2026-06-09T20:00:00Z",
  "device_id": "…uuid…",
  "schema_version": 1
}
```

`decision` values: `audit_allow`, `warn_proceed`, `warn_cancel`, `block`,
`block_sensitive`.

---

## Migration notes (legacy JS-listener branch)

The isolated-world JS listener / gin bridge / renderer Mojo path is
**superseded** by this native hook and should be deleted when that branch is
merged. For parity validation, run both temporarily and compare
`detection_source: "native"` vs `"dom"` in Datadog; native should be a strict
superset (it also catches context-menu and app-menu pastes). If the JS
listener is kept temporarily, move it off `ISOLATED_WORLD_ID_CHROME_INTERNAL`
onto the new `ISOLATED_WORLD_ID_OASIS_TELEMETRY`.

---

## How to test

```bash
# 1. Start the relay (dev mode, localhost is allowed without TLS)
cd oasis-relay
cp config.example.json config.json   # set sinks: [{"type":"stdout"}]
node server.js config.json

# 2. Launch the build with dev overrides
out/Default/Chromium.app/Contents/MacOS/Chromium \
  --oasis-relay-url=http://127.0.0.1:8443

# (enable the feature via policy/prefs: OasisTelemetryEnabled=true)

# 3. Paste into chatgpt.com / claude.ai / gemini.google.com
#    → relay stdout shows sanitized events

# Unit tests
autoninja -C out/Default unit_tests
out/Default/unit_tests --gtest_filter='*Oasis*:*LlmProvider*'
```

Verified so far: all new and touched translation units compile
(`autoninja`), policy YAML passes Chromium's policy codegen, `gn format`
clean, relay auth lifecycle tested end-to-end (enroll → upload → rotate →
revoke → 401 → re-enroll). Note: linking the full `unit_tests` binary
currently fails on a **pre-existing** macOS SDK issue in
`ui/gfx/image/image_unittest_util_apple.mm` (`kCGImageByteOrder32Host`),
unrelated to these changes.

---

## Production checklist (before customer-facing deployment)

- [ ] Host the relay behind a real CA-signed certificate
- [ ] Gate `/v1/enroll` on an MDM-delivered enrollment secret or network allowlist
- [ ] Deliver policies via your management channel (MDM / managed prefs)
- [ ] Configure sink retention (e.g. Datadog log retention) per tenant policy
- [ ] Decide warn/block rollout per domain (`OasisPasteEnforcementDomainOverrides`)
- [ ] Remove the legacy JS listener after parity validation
