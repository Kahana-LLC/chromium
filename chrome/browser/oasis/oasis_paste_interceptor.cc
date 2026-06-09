// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/oasis/oasis_paste_interceptor.h"

#include <optional>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/oasis/llm_provider_registry.h"
#include "chrome/browser/oasis/oasis_paste_policy.h"
#include "chrome/browser/oasis/oasis_paste_telemetry_event.h"
#include "chrome/browser/oasis/oasis_pref_names.h"
#include "chrome/browser/oasis/oasis_sensitive_data_classifier.h"
#include "chrome/browser/oasis/oasis_telemetry_service.h"
#include "chrome/browser/oasis/oasis_telemetry_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "crypto/hash.h"
#include "ui/base/clipboard/clipboard_metadata.h"
#include "url/gurl.h"
#include "url/origin.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/simple_message_box.h"
#endif

namespace oasis {

namespace {

void RecordEvent(Profile* profile,
                 const OasisPasteTelemetryEvent& event) {
  if (OasisTelemetryService* service =
          OasisTelemetryServiceFactory::GetForProfile(profile)) {
    service->RecordPasteEvent(event);
  }
}

#if !BUILDFLAG(IS_ANDROID)
void OnWarnDialogClosed(
    Profile* profile,
    OasisPasteTelemetryEvent event,
    content::ClipboardPasteData clipboard_paste_data,
    OasisPasteInterceptor::ContinuePasteCallback continue_paste,
    content::ContentBrowserClient::IsClipboardPasteAllowedCallback callback,
    chrome::MessageBoxResult result) {
  if (result == chrome::MESSAGE_BOX_RESULT_YES) {
    event.decision = "warn_proceed";
    RecordEvent(profile, event);
    std::move(continue_paste)
        .Run(std::move(clipboard_paste_data), std::move(callback));
  } else {
    event.decision = "warn_cancel";
    RecordEvent(profile, event);
    std::move(callback).Run(std::nullopt);
  }
}
#endif  // !BUILDFLAG(IS_ANDROID)

}  // namespace

// static
void OasisPasteInterceptor::EvaluatePaste(
    const content::ClipboardEndpoint& source,
    const content::ClipboardEndpoint& destination,
    const ui::ClipboardMetadata& metadata,
    content::ClipboardPasteData clipboard_paste_data,
    ContinuePasteCallback continue_paste,
    content::ContentBrowserClient::IsClipboardPasteAllowedCallback callback) {
  content::WebContents* web_contents = destination.web_contents();
  Profile* profile =
      destination.browser_context()
          ? Profile::FromBrowserContext(destination.browser_context())
          : nullptr;
  if (!web_contents || !profile) {
    std::move(continue_paste)
        .Run(std::move(clipboard_paste_data), std::move(callback));
    return;
  }

  const PrefService* pref_service = profile->GetPrefs();
  if (!pref_service->GetBoolean(prefs::kOasisTelemetryEnabled)) {
    std::move(continue_paste)
        .Run(std::move(clipboard_paste_data), std::move(callback));
    return;
  }

  // All trusted destination fields are derived from the committed frame
  // URL in the browser process; nothing renderer-supplied is used.
  const GURL& url =
      web_contents->GetPrimaryMainFrame()->GetLastCommittedURL();
  std::optional<LlmProviderMatch> match =
      LlmProviderRegistry::Match(url, pref_service);
  if (!match) {
    std::move(continue_paste)
        .Run(std::move(clipboard_paste_data), std::move(callback));
    return;
  }

  const std::string utf8_text = base::UTF16ToUTF8(clipboard_paste_data.text);

  OasisPasteTelemetryEvent event;
  event.provider = match->provider;
  event.domain = match->matched_pattern;
  event.origin = url::Origin::Create(url).Serialize();
  event.content_length_bucket = OasisPasteTelemetryEvent::BucketForLength(
      !utf8_text.empty() ? utf8_text.size()
                         : metadata.size.value_or(clipboard_paste_data.size()));
  if (pref_service->GetBoolean(prefs::kOasisTelemetryIncludeContentHash) &&
      !utf8_text.empty()) {
    const std::array<uint8_t, crypto::hash::kSha256Size> digest =
        crypto::hash::Sha256(utf8_text);
    event.content_hash = base::ToLowerASCII(base::HexEncode(digest));
  }
  event.classifications = ClassifySensitiveData(utf8_text);

  PasteEnforcementMode mode =
      ResolvePasteEnforcementMode(pref_service, match->matched_pattern);

  // Sensitive content escalates to a block regardless of the configured
  // mode when OasisPasteBlockSensitiveData is enabled.
  const bool block_sensitive =
      !event.classifications.empty() &&
      pref_service->GetBoolean(prefs::kOasisPasteBlockSensitiveData);

  event.enforcement_mode = PasteEnforcementModeToString(mode);

  if (block_sensitive) {
    event.decision = "block_sensitive";
    RecordEvent(profile, event);
    std::move(callback).Run(std::nullopt);
    return;
  }

  switch (mode) {
    case PasteEnforcementMode::kAudit:
      event.decision = "audit_allow";
      RecordEvent(profile, event);
      std::move(continue_paste)
          .Run(std::move(clipboard_paste_data), std::move(callback));
      return;

    case PasteEnforcementMode::kWarn: {
#if BUILDFLAG(IS_ANDROID)
      // No modal warn surface on Android yet; degrade to audit.
      event.decision = "audit_allow";
      RecordEvent(profile, event);
      std::move(continue_paste)
          .Run(std::move(clipboard_paste_data), std::move(callback));
#else
      const std::u16string message = base::UTF8ToUTF16(base::StrCat(
          {"You are pasting into an AI tool (", match->matched_pattern,
           ").\n\nYour organization monitors data shared with generative "
           "AI services. Continue?"}));
      chrome::ShowQuestionMessageBoxAsync(
          web_contents->GetTopLevelNativeWindow(), u"Oasis Security",
          message,
          base::BindOnce(&OnWarnDialogClosed, profile, std::move(event),
                         std::move(clipboard_paste_data),
                         std::move(continue_paste), std::move(callback)));
#endif
      return;
    }

    case PasteEnforcementMode::kBlock:
      event.decision = "block";
      RecordEvent(profile, event);
      std::move(callback).Run(std::nullopt);
      return;
  }
}

}  // namespace oasis
