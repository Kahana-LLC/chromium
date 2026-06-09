// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/oasis/oasis_paste_policy.h"

#include "base/values.h"
#include "chrome/browser/oasis/oasis_pref_names.h"
#include "components/prefs/pref_service.h"

namespace oasis {

namespace {

PasteEnforcementMode ParseMode(const std::string& value) {
  if (value == "warn") {
    return PasteEnforcementMode::kWarn;
  }
  if (value == "block") {
    return PasteEnforcementMode::kBlock;
  }
  return PasteEnforcementMode::kAudit;
}

}  // namespace

PasteEnforcementMode ResolvePasteEnforcementMode(
    const PrefService* pref_service,
    const std::string& matched_pattern) {
  if (!pref_service) {
    return PasteEnforcementMode::kAudit;
  }

  const base::Value::Dict& overrides =
      pref_service->GetDict(prefs::kOasisPasteEnforcementDomainOverrides);
  if (const std::string* override_mode =
          overrides.FindString(matched_pattern)) {
    return ParseMode(*override_mode);
  }

  return ParseMode(
      pref_service->GetString(prefs::kOasisPasteEnforcementMode));
}

const char* PasteEnforcementModeToString(PasteEnforcementMode mode) {
  switch (mode) {
    case PasteEnforcementMode::kAudit:
      return "audit";
    case PasteEnforcementMode::kWarn:
      return "warn";
    case PasteEnforcementMode::kBlock:
      return "block";
  }
}

}  // namespace oasis
