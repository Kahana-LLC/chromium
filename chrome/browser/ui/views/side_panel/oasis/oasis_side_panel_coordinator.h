// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_OASIS_OASIS_SIDE_PANEL_COORDINATOR_H_
#define CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_OASIS_OASIS_SIDE_PANEL_COORDINATOR_H_

#include <memory>

class SidePanelEntryScope;
class SidePanelRegistry;

namespace views {
class View;
}  // namespace views

// OasisSidePanelCoordinator handles the creation and registration of the
// Oasis AI SidePanelEntry.
class OasisSidePanelCoordinator {
 public:
  OasisSidePanelCoordinator();
  ~OasisSidePanelCoordinator() = default;

  void CreateAndRegisterEntry(SidePanelRegistry* global_registry);

 private:
  std::unique_ptr<views::View> CreateOasisWebView(SidePanelEntryScope& scope);
};

#endif  // CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_OASIS_OASIS_SIDE_PANEL_COORDINATOR_H_
