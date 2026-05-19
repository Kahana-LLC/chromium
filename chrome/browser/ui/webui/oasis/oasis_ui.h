// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_OASIS_OASIS_UI_H_
#define CHROME_BROWSER_UI_WEBUI_OASIS_OASIS_UI_H_

#include <string_view>

#include "chrome/browser/ui/webui/top_chrome/top_chrome_web_ui_controller.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_webui_config.h"
#include "chrome/common/webui_url_constants.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/url_constants.h"

class OasisUI;

class OasisUIConfig : public DefaultTopChromeWebUIConfig<OasisUI> {
 public:
  OasisUIConfig();
};

// WebUI controller for the Oasis AI side panel.
class OasisUI : public TopChromeWebUIController,
                public content::WebContentsObserver {
 public:
  explicit OasisUI(content::WebUI* web_ui);
  OasisUI(const OasisUI&) = delete;
  OasisUI& operator=(const OasisUI&) = delete;
  ~OasisUI() override;

  static constexpr std::string_view GetWebUIName() { return "OasisAI"; }

  // content::WebContentsObserver:
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

#endif  // CHROME_BROWSER_UI_WEBUI_OASIS_OASIS_UI_H_
