// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OASIS_OASIS_PASTE_POLICY_H_
#define CHROME_BROWSER_OASIS_OASIS_PASTE_POLICY_H_

#include <string>

class PrefService;

namespace oasis {

// Enforcement mode for pastes into monitored LLM sites.
enum class PasteEnforcementMode {
  // Allow the paste and record telemetry.
  kAudit,
  // Show a confirmation dialog; the user may proceed or cancel.
  kWarn,
  // Deny the paste.
  kBlock,
};

// Resolves the effective enforcement mode for `matched_pattern` (the
// registry host pattern that matched the destination frame). Per-domain
// overrides from the OasisPasteEnforcementDomainOverrides policy take
// precedence over the global OasisPasteEnforcementMode policy. Unknown
// values fall back to audit. User/group targeting is expressed by the
// management server mapping groups to different policy values; the browser
// only ever sees the resolved per-profile policy.
PasteEnforcementMode ResolvePasteEnforcementMode(
    const PrefService* prefs,
    const std::string& matched_pattern);

const char* PasteEnforcementModeToString(PasteEnforcementMode mode);

}  // namespace oasis

#endif  // CHROME_BROWSER_OASIS_OASIS_PASTE_POLICY_H_
