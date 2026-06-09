// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OASIS_OASIS_TELEMETRY_SERVICE_FACTORY_H_
#define CHROME_BROWSER_OASIS_OASIS_TELEMETRY_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class Profile;

namespace oasis {

class OasisTelemetryService;

class OasisTelemetryServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static OasisTelemetryService* GetForProfile(Profile* profile);
  static OasisTelemetryServiceFactory* GetInstance();

  OasisTelemetryServiceFactory(const OasisTelemetryServiceFactory&) = delete;
  OasisTelemetryServiceFactory& operator=(const OasisTelemetryServiceFactory&) =
      delete;

 private:
  friend class base::NoDestructor<OasisTelemetryServiceFactory>;

  OasisTelemetryServiceFactory();
  ~OasisTelemetryServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace oasis

#endif  // CHROME_BROWSER_OASIS_OASIS_TELEMETRY_SERVICE_FACTORY_H_
