// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/oasis/oasis_paste_telemetry_event.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace oasis {

TEST(OasisPasteTelemetryEventTest, BucketsAreCoarse) {
  EXPECT_EQ(OasisPasteTelemetryEvent::BucketForLength(0), "0-100");
  EXPECT_EQ(OasisPasteTelemetryEvent::BucketForLength(100), "0-100");
  EXPECT_EQ(OasisPasteTelemetryEvent::BucketForLength(101), "100-1k");
  EXPECT_EQ(OasisPasteTelemetryEvent::BucketForLength(1000), "100-1k");
  EXPECT_EQ(OasisPasteTelemetryEvent::BucketForLength(1001), "1k-10k");
  EXPECT_EQ(OasisPasteTelemetryEvent::BucketForLength(10000), "1k-10k");
  EXPECT_EQ(OasisPasteTelemetryEvent::BucketForLength(10001), "10k+");
}

TEST(OasisPasteTelemetryEventTest, ToDictOmitsEmptyHashAndClassifications) {
  OasisPasteTelemetryEvent event;
  event.provider = "openai";
  event.domain = "chatgpt.com";
  event.origin = "https://chatgpt.com";
  event.content_length_bucket = "0-100";
  event.enforcement_mode = "audit";
  event.decision = "audit_allow";

  base::Value::Dict dict = event.ToDict();
  EXPECT_EQ(*dict.FindString("event_type"), "llm_paste");
  EXPECT_EQ(*dict.FindString("provider"), "openai");
  EXPECT_EQ(*dict.FindString("detection_source"), "native");
  EXPECT_FALSE(dict.contains("content_hash"));
  EXPECT_FALSE(dict.contains("classifications"));
}

TEST(OasisPasteTelemetryEventTest, ToDictIncludesOptionalFieldsWhenSet) {
  OasisPasteTelemetryEvent event;
  event.content_hash = "abc123";
  event.classifications = {"ssn", "credit_card"};

  base::Value::Dict dict = event.ToDict();
  EXPECT_EQ(*dict.FindString("content_hash"), "abc123");
  ASSERT_TRUE(dict.FindList("classifications"));
  EXPECT_EQ(dict.FindList("classifications")->size(), 2u);
}

}  // namespace oasis
