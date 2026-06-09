// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OASIS_OASIS_PASTE_TELEMETRY_EVENT_H_
#define CHROME_BROWSER_OASIS_OASIS_PASTE_TELEMETRY_EVENT_H_

#include <string>
#include <vector>

#include "base/values.h"

namespace oasis {

// Privacy-reduced description of a paste into a monitored LLM site. Raw
// pasted content is never stored here; see chrome/browser/oasis/PRIVACY.md.
struct OasisPasteTelemetryEvent {
  OasisPasteTelemetryEvent();
  OasisPasteTelemetryEvent(const OasisPasteTelemetryEvent&);
  OasisPasteTelemetryEvent& operator=(const OasisPasteTelemetryEvent&);
  OasisPasteTelemetryEvent(OasisPasteTelemetryEvent&&);
  OasisPasteTelemetryEvent& operator=(OasisPasteTelemetryEvent&&);
  ~OasisPasteTelemetryEvent();

  // Canonical provider identifier from the LlmProviderRegistry.
  std::string provider;

  // The registry pattern that matched, e.g. "chatgpt.com".
  std::string domain;

  // Serialized origin (scheme://host[:port]) of the destination frame.
  // Never a full URL with path or query.
  std::string origin;

  // Coarse content size: "0-100", "100-1k", "1k-10k" or "10k+".
  std::string content_length_bucket;

  // Lowercase hex SHA-256 of the pasted text. Empty unless the
  // OasisTelemetryIncludeContentHash policy is enabled.
  std::string content_hash;

  // Where the paste was observed. Always "native" for the browser-process
  // clipboard hook; "dom" is reserved for the legacy isolated-world
  // listener during parity validation.
  std::string detection_source = "native";

  // Effective enforcement mode for this paste: "audit", "warn" or "block".
  std::string enforcement_mode;

  // Outcome: "audit_allow", "warn_proceed", "warn_cancel", "block" or
  // "block_sensitive".
  std::string decision;

  // Sensitive-data categories detected by the on-device classifier.
  std::vector<std::string> classifications;

  // Returns the JSON representation uploaded to the relay. Timestamp and
  // device identity fields are appended by OasisTelemetryService.
  base::Value::Dict ToDict() const;

  // Maps a byte count to a coarse bucket label.
  static std::string BucketForLength(size_t length);
};

}  // namespace oasis

#endif  // CHROME_BROWSER_OASIS_OASIS_PASTE_TELEMETRY_EVENT_H_
