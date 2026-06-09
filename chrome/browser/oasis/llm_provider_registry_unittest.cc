// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/oasis/llm_provider_registry.h"

#include "base/values.h"
#include "chrome/browser/oasis/oasis_pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace oasis {

class LlmProviderRegistryTest : public testing::Test {
 protected:
  LlmProviderRegistryTest() {
    RegisterProfilePrefs(prefs_.registry());
  }

  sync_preferences::TestingPrefServiceSyncable prefs_;
};

TEST_F(LlmProviderRegistryTest, MatchesBuiltInProviders) {
  auto match =
      LlmProviderRegistry::Match(GURL("https://chatgpt.com/c/123"), &prefs_);
  ASSERT_TRUE(match);
  EXPECT_EQ(match->provider, "openai");
  EXPECT_EQ(match->matched_pattern, "chatgpt.com");

  match = LlmProviderRegistry::Match(GURL("https://claude.ai/new"), &prefs_);
  ASSERT_TRUE(match);
  EXPECT_EQ(match->provider, "anthropic");
}

TEST_F(LlmProviderRegistryTest, MatchesSubdomains) {
  auto match =
      LlmProviderRegistry::Match(GURL("https://www.chatgpt.com/"), &prefs_);
  ASSERT_TRUE(match);
  EXPECT_EQ(match->provider, "openai");
  EXPECT_EQ(match->host, "www.chatgpt.com");
}

TEST_F(LlmProviderRegistryTest, SpecificHostPatternsDoNotMatchParentDomain) {
  // gemini.google.com is monitored; the rest of google.com is not.
  EXPECT_TRUE(LlmProviderRegistry::Match(GURL("https://gemini.google.com/app"),
                                         &prefs_));
  EXPECT_FALSE(
      LlmProviderRegistry::Match(GURL("https://mail.google.com/"), &prefs_));
  EXPECT_FALSE(
      LlmProviderRegistry::Match(GURL("https://google.com/"), &prefs_));
}

TEST_F(LlmProviderRegistryTest, RejectsLookalikeHosts) {
  EXPECT_FALSE(
      LlmProviderRegistry::Match(GURL("https://notchatgpt.com/"), &prefs_));
  EXPECT_FALSE(
      LlmProviderRegistry::Match(GURL("https://chatgpt.com.evil.example/"),
                                 &prefs_));
}

TEST_F(LlmProviderRegistryTest, IgnoresNonHttpSchemes) {
  EXPECT_FALSE(
      LlmProviderRegistry::Match(GURL("ftp://chatgpt.com/"), &prefs_));
  EXPECT_FALSE(LlmProviderRegistry::Match(GURL(), &prefs_));
}

TEST_F(LlmProviderRegistryTest, MatchesPolicySuppliedDomains) {
  base::Value::List domains;
  domains.Append("llm.internal.example.com=internal-llm");
  domains.Append("chat.partner.example.org");
  prefs_.SetList(prefs::kOasisTelemetryMonitoredDomains, std::move(domains));

  auto match = LlmProviderRegistry::Match(
      GURL("https://llm.internal.example.com/chat"), &prefs_);
  ASSERT_TRUE(match);
  EXPECT_EQ(match->provider, "internal-llm");
  EXPECT_EQ(match->matched_pattern, "llm.internal.example.com");

  match = LlmProviderRegistry::Match(
      GURL("https://chat.partner.example.org/"), &prefs_);
  ASSERT_TRUE(match);
  EXPECT_EQ(match->provider, "custom");
}

}  // namespace oasis
