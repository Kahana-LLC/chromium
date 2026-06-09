// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OASIS_OASIS_PREF_NAMES_H_
#define CHROME_BROWSER_OASIS_OASIS_PREF_NAMES_H_

class PrefRegistrySimple;

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace oasis {
namespace prefs {

// Profile prefs, backed by the OasisTelemetry* enterprise policies.

// Boolean. Master switch for Oasis LLM paste telemetry.
extern const char kOasisTelemetryEnabled[];

// String. HTTPS URL of the Oasis telemetry relay. Non-HTTPS values are
// rejected at read time.
extern const char kOasisTelemetryRelayUrl[];

// List of strings. Additional monitored domains, merged with the built-in
// provider registry. Entries are either "host" or "host=provider".
extern const char kOasisTelemetryMonitoredDomains[];

// Integer. Seconds between telemetry batch uploads.
extern const char kOasisTelemetryBatchIntervalSeconds[];

// Integer. Maximum number of events per batch before an immediate flush.
extern const char kOasisTelemetryBatchMaxEvents[];

// Boolean. When true, a SHA-256 hash of pasted text is included in telemetry.
// Defaults to false; see chrome/browser/oasis/PRIVACY.md.
extern const char kOasisTelemetryIncludeContentHash[];

// String. Paste enforcement mode: "audit", "warn" or "block".
extern const char kOasisPasteEnforcementMode[];

// Dict. Per-domain enforcement mode overrides, mapping a monitored host
// pattern to "audit", "warn" or "block".
extern const char kOasisPasteEnforcementDomainOverrides[];

// Boolean. When true, pastes classified as containing sensitive data are
// blocked even in audit/warn mode.
extern const char kOasisPasteBlockSensitiveData[];

// Local-state prefs (device identity, not policy-controlled).

// String. Stable random device identifier generated on first run.
extern const char kOasisDeviceId[];

// String. Bearer token issued by the relay at enrollment.
extern const char kOasisDeviceToken[];

// Time (stored as int64 internal value). Last token rotation time.
extern const char kOasisDeviceTokenRotatedAt[];

}  // namespace prefs

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);
void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

}  // namespace oasis

#endif  // CHROME_BROWSER_OASIS_OASIS_PREF_NAMES_H_
