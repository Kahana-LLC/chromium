// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/oasis/oasis_ui.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/grit/side_panel_oasis_resources.h"
#include "chrome/grit/side_panel_oasis_resources_map.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "ui/webui/webui_util.h"

OasisUIConfig::OasisUIConfig()
    : DefaultTopChromeWebUIConfig(content::kChromeUIScheme,
                                  chrome::kChromeUIOasisAIHost) {}

OasisUI::OasisUI(content::WebUI* web_ui)
    : TopChromeWebUIController(web_ui) {
  Profile* const profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource* source = content::WebUIDataSource::CreateAndAdd(
      profile, chrome::kChromeUIOasisAIHost);
  webui::SetupWebUIDataSource(source, kSidePanelOasisResources,
                              IDR_SIDE_PANEL_OASIS_OASIS_HTML);
  WebContentsObserver::Observe(web_ui->GetWebContents());
}

OasisUI::~OasisUI() = default;

void OasisUI::DidFinishLoad(content::RenderFrameHost* render_frame_host,
                            const GURL& validated_url) {
  if (!render_frame_host->IsInPrimaryMainFrame()) {
    return;
  }
  if (auto emb = embedder()) {
    emb->ShowUI();
  }
}

WEB_UI_CONTROLLER_TYPE_IMPL(OasisUI)
