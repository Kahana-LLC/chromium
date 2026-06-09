// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/oasis/llm_provider_registry.h"

#include <string_view>
#include <utility>

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "chrome/browser/oasis/oasis_pref_names.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

namespace oasis {

namespace {

struct BuiltInProvider {
  std::string_view host_pattern;
  std::string_view provider;
};

// Built-in monitored LLM destinations. Patterns match the exact host or any
// subdomain of it. Keep host patterns as specific as possible; never list a
// bare eTLD+1 for multi-product domains (e.g. google.com).
constexpr BuiltInProvider kBuiltInProviders[] = {
    {"chatgpt.com", "openai"},
    {"chat.openai.com", "openai"},
    {"claude.ai", "anthropic"},
    {"gemini.google.com", "google"},
    {"aistudio.google.com", "google"},
    {"copilot.microsoft.com", "microsoft"},
    {"perplexity.ai", "perplexity"},
    {"chat.deepseek.com", "deepseek"},
    {"chat.mistral.ai", "mistral"},
    {"grok.com", "xai"},
    {"meta.ai", "meta"},
    {"poe.com", "poe"},
};

bool HostMatchesPattern(std::string_view host, std::string_view pattern) {
  if (host == pattern) {
    return true;
  }
  return host.size() > pattern.size() + 1 &&
         base::EndsWith(host, pattern, base::CompareCase::SENSITIVE) &&
         host[host.size() - pattern.size() - 1] == '.';
}

}  // namespace

// static
std::optional<LlmProviderMatch> LlmProviderRegistry::Match(
    const GURL& url,
    const PrefService* prefs) {
  if (!url.is_valid() || !url.SchemeIsHTTPOrHTTPS()) {
    return std::nullopt;
  }

  const std::string host = base::ToLowerASCII(url.host());

  for (const auto& entry : kBuiltInProviders) {
    if (HostMatchesPattern(host, entry.host_pattern)) {
      return LlmProviderMatch{std::string(entry.provider),
                              std::string(entry.host_pattern), host};
    }
  }

  if (!prefs) {
    return std::nullopt;
  }

  // Policy-supplied entries are "host" or "host=provider".
  for (const base::Value& value :
       prefs->GetList(prefs::kOasisTelemetryMonitoredDomains)) {
    const std::string* raw = value.GetIfString();
    if (!raw || raw->empty()) {
      continue;
    }
    std::vector<std::string_view> parts = base::SplitStringPiece(
        *raw, "=", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    if (parts.empty()) {
      continue;
    }
    const std::string pattern = base::ToLowerASCII(parts[0]);
    if (HostMatchesPattern(host, pattern)) {
      std::string provider =
          parts.size() > 1 ? std::string(parts[1]) : "custom";
      return LlmProviderMatch{std::move(provider), pattern, host};
    }
  }

  return std::nullopt;
}

}  // namespace oasis
