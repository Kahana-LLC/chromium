// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/side_panel/oasis/oasis_side_panel_coordinator.h"

#include "base/functional/bind.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry_id.h"
#include "chrome/browser/ui/views/side_panel/side_panel_registry.h"
#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"
#include "chrome/browser/ui/webui/oasis/oasis_ui.h"
#include "chrome/browser/ui/webui/top_chrome/webui_contents_wrapper.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "url/gurl.h"

using SidePanelWebUIViewT_OasisUI = SidePanelWebUIViewT<OasisUI>;
BEGIN_TEMPLATE_METADATA(SidePanelWebUIViewT_OasisUI, SidePanelWebUIViewT)
END_METADATA

OasisSidePanelCoordinator::OasisSidePanelCoordinator() = default;

void OasisSidePanelCoordinator::CreateAndRegisterEntry(
    SidePanelRegistry* global_registry) {
  global_registry->Register(std::make_unique<SidePanelEntry>(
      SidePanelEntry::Key(SidePanelEntry::Id::kOasisAI),
      base::BindRepeating(&OasisSidePanelCoordinator::CreateOasisWebView,
                          base::Unretained(this)),
      /*default_content_width_callback=*/base::NullCallback()));
}

std::unique_ptr<views::View>
OasisSidePanelCoordinator::CreateOasisWebView(SidePanelEntryScope& scope) {
  return std::make_unique<SidePanelWebUIViewT<OasisUI>>(
      scope, base::RepeatingClosure(), base::RepeatingClosure(),
      std::make_unique<WebUIContentsWrapperT<OasisUI>>(
          GURL(chrome::kChromeUIOasisAIURL),
          scope.GetBrowserWindowInterface().GetProfile(),
          IDS_OASIS_AI_TITLE,
          /*esc_closes_ui=*/false));
}
