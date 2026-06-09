// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/oasis/oasis_pref_names.h"

#include "base/time/time.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"

namespace oasis {
namespace prefs {

const char kOasisTelemetryEnabled[] = "oasis.telemetry.enabled";
const char kOasisTelemetryRelayUrl[] = "oasis.telemetry.relay_url";
const char kOasisTelemetryMonitoredDomains[] =
    "oasis.telemetry.monitored_domains";
const char kOasisTelemetryBatchIntervalSeconds[] =
    "oasis.telemetry.batch_interval_seconds";
const char kOasisTelemetryBatchMaxEvents[] =
    "oasis.telemetry.batch_max_events";
const char kOasisTelemetryIncludeContentHash[] =
    "oasis.telemetry.include_content_hash";
const char kOasisPasteEnforcementMode[] = "oasis.paste.enforcement_mode";
const char kOasisPasteEnforcementDomainOverrides[] =
    "oasis.paste.enforcement_domain_overrides";
const char kOasisPasteBlockSensitiveData[] =
    "oasis.paste.block_sensitive_data";

const char kOasisDeviceId[] = "oasis.device.id";
const char kOasisDeviceToken[] = "oasis.device.token";
const char kOasisDeviceTokenRotatedAt[] = "oasis.device.token_rotated_at";

}  // namespace prefs

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(prefs::kOasisTelemetryEnabled, false);
  registry->RegisterStringPref(prefs::kOasisTelemetryRelayUrl, std::string());
  registry->RegisterListPref(prefs::kOasisTelemetryMonitoredDomains);
  registry->RegisterIntegerPref(prefs::kOasisTelemetryBatchIntervalSeconds,
                                10);
  registry->RegisterIntegerPref(prefs::kOasisTelemetryBatchMaxEvents, 20);
  registry->RegisterBooleanPref(prefs::kOasisTelemetryIncludeContentHash,
                                false);
  registry->RegisterStringPref(prefs::kOasisPasteEnforcementMode, "audit");
  registry->RegisterDictionaryPref(
      prefs::kOasisPasteEnforcementDomainOverrides);
  registry->RegisterBooleanPref(prefs::kOasisPasteBlockSensitiveData, false);
}

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(prefs::kOasisDeviceId, std::string());
  registry->RegisterStringPref(prefs::kOasisDeviceToken, std::string());
  registry->RegisterTimePref(prefs::kOasisDeviceTokenRotatedAt, base::Time());
}

}  // namespace oasis
