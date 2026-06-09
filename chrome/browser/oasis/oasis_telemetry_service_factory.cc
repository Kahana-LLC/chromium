// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/oasis/oasis_telemetry_service_factory.h"

#include "chrome/browser/oasis/oasis_telemetry_service.h"
#include "chrome/browser/profiles/profile.h"

namespace oasis {

// static
OasisTelemetryService* OasisTelemetryServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<OasisTelemetryService*>(
      GetInstance()->GetServiceForBrowserContext(profile, /*create=*/true));
}

// static
OasisTelemetryServiceFactory* OasisTelemetryServiceFactory::GetInstance() {
  static base::NoDestructor<OasisTelemetryServiceFactory> instance;
  return instance.get();
}

OasisTelemetryServiceFactory::OasisTelemetryServiceFactory()
    : ProfileKeyedServiceFactory(
          "OasisTelemetryService",
          // Telemetry is an enterprise visibility feature; do not observe
          // incognito or guest sessions.
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOriginalOnly)
              .WithGuest(ProfileSelection::kNone)
              .WithSystem(ProfileSelection::kNone)
              .Build()) {}

OasisTelemetryServiceFactory::~OasisTelemetryServiceFactory() = default;

std::unique_ptr<KeyedService>
OasisTelemetryServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<OasisTelemetryService>(
      Profile::FromBrowserContext(context));
}

}  // namespace oasis
