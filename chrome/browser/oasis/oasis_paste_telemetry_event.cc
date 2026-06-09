// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/oasis/oasis_paste_telemetry_event.h"

namespace oasis {

OasisPasteTelemetryEvent::OasisPasteTelemetryEvent() = default;
OasisPasteTelemetryEvent::OasisPasteTelemetryEvent(
    const OasisPasteTelemetryEvent&) = default;
OasisPasteTelemetryEvent& OasisPasteTelemetryEvent::operator=(
    const OasisPasteTelemetryEvent&) = default;
OasisPasteTelemetryEvent::OasisPasteTelemetryEvent(OasisPasteTelemetryEvent&&) =
    default;
OasisPasteTelemetryEvent& OasisPasteTelemetryEvent::operator=(
    OasisPasteTelemetryEvent&&) = default;
OasisPasteTelemetryEvent::~OasisPasteTelemetryEvent() = default;

base::Value::Dict OasisPasteTelemetryEvent::ToDict() const {
  base::Value::Dict dict;
  dict.Set("event_type", "llm_paste");
  dict.Set("provider", provider);
  dict.Set("domain", domain);
  dict.Set("origin", origin);
  dict.Set("content_length_bucket", content_length_bucket);
  if (!content_hash.empty()) {
    dict.Set("content_hash", content_hash);
  }
  dict.Set("detection_source", detection_source);
  dict.Set("enforcement_mode", enforcement_mode);
  dict.Set("decision", decision);
  if (!classifications.empty()) {
    base::Value::List list;
    for (const std::string& category : classifications) {
      list.Append(category);
    }
    dict.Set("classifications", std::move(list));
  }
  return dict;
}

// static
std::string OasisPasteTelemetryEvent::BucketForLength(size_t length) {
  if (length <= 100) {
    return "0-100";
  }
  if (length <= 1000) {
    return "100-1k";
  }
  if (length <= 10000) {
    return "1k-10k";
  }
  return "10k+";
}

}  // namespace oasis
