// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OASIS_OASIS_PASTE_INTERCEPTOR_H_
#define CHROME_BROWSER_OASIS_OASIS_PASTE_INTERCEPTOR_H_

#include "base/functional/callback.h"
#include "content/public/browser/clipboard_types.h"
#include "content/public/browser/content_browser_client.h"

namespace ui {
struct ClipboardMetadata;
}

namespace oasis {

// Native browser-process paste hook for Oasis LLM telemetry and
// enforcement. Invoked from
// ChromeContentBrowserClient::IsClipboardPasteAllowedByPolicy(), which sits
// on the ClipboardHostImpl paste path, so every paste (keyboard, context
// menu, app menu) is observed before content is delivered to the page.
//
// The renderer plays no role in detection: destination provider, domain
// and origin are derived in the browser process from the committed frame
// URL via LlmProviderRegistry.
class OasisPasteInterceptor {
 public:
  // Runs the pre-existing enterprise paste policy flow with the (possibly
  // unmodified) paste data and the final completion callback.
  using ContinuePasteCallback = base::OnceCallback<void(
      content::ClipboardPasteData,
      content::ContentBrowserClient::IsClipboardPasteAllowedCallback)>;

  // Evaluates a paste into `destination`:
  //  - Destination not a monitored LLM site, or telemetry disabled by
  //    policy: `continue_paste` runs immediately.
  //  - Audit mode: telemetry is recorded and `continue_paste` runs.
  //  - Warn mode: a confirmation dialog is shown; the user's decision is
  //    recorded and either `continue_paste` runs or `callback` is invoked
  //    with std::nullopt (paste denied).
  //  - Block mode (or sensitive data with OasisPasteBlockSensitiveData):
  //    telemetry is recorded and `callback` is invoked with std::nullopt.
  static void EvaluatePaste(
      const content::ClipboardEndpoint& source,
      const content::ClipboardEndpoint& destination,
      const ui::ClipboardMetadata& metadata,
      content::ClipboardPasteData clipboard_paste_data,
      ContinuePasteCallback continue_paste,
      content::ContentBrowserClient::IsClipboardPasteAllowedCallback callback);
};

}  // namespace oasis

#endif  // CHROME_BROWSER_OASIS_OASIS_PASTE_INTERCEPTOR_H_
