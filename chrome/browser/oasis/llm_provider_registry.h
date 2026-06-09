// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OASIS_LLM_PROVIDER_REGISTRY_H_
#define CHROME_BROWSER_OASIS_LLM_PROVIDER_REGISTRY_H_

#include <optional>
#include <string>

class GURL;
class PrefService;

namespace oasis {

// Result of matching a frame URL against the monitored LLM provider list.
struct LlmProviderMatch {
  // Canonical provider identifier, e.g. "openai", "anthropic", "google".
  // Policy-supplied domains without an explicit provider map to "custom".
  std::string provider;

  // The host pattern that matched, e.g. "chatgpt.com".
  std::string matched_pattern;

  // The actual host of the committed frame URL, e.g. "chatgpt.com".
  std::string host;
};

// Single source of truth for which destinations count as monitored LLM
// sites. All provider/domain knowledge lives here, in the browser process.
// Renderers never carry or report provider metadata; trusted fields are
// always derived from the committed frame URL via this registry.
class LlmProviderRegistry {
 public:
  // Matches `url` against the built-in provider table plus any additional
  // domains supplied through the OasisTelemetryMonitoredDomains policy
  // (read from `prefs`, which may be null to match built-ins only).
  //
  // A pattern matches when the URL host equals the pattern or is a
  // subdomain of it. Matching is host-suffix based rather than eTLD+1
  // based so that entries like "gemini.google.com" do not match all of
  // google.com.
  static std::optional<LlmProviderMatch> Match(const GURL& url,
                                               const PrefService* prefs);
};

}  // namespace oasis

#endif  // CHROME_BROWSER_OASIS_LLM_PROVIDER_REGISTRY_H_
