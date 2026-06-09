// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OASIS_OASIS_TELEMETRY_SERVICE_H_
#define CHROME_BROWSER_OASIS_OASIS_TELEMETRY_SERVICE_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "chrome/browser/oasis/oasis_paste_telemetry_event.h"
#include "components/keyed_service/core/keyed_service.h"

class GURL;
class Profile;

namespace network {
class SimpleURLLoader;
}

namespace oasis {

// Per-profile service that batches privacy-reduced paste telemetry events
// and uploads them to the enterprise-configured Oasis relay. The relay (not
// the browser) holds third-party sink credentials such as the Datadog API
// key.
class OasisTelemetryService : public KeyedService {
 public:
  explicit OasisTelemetryService(Profile* profile);
  OasisTelemetryService(const OasisTelemetryService&) = delete;
  OasisTelemetryService& operator=(const OasisTelemetryService&) = delete;
  ~OasisTelemetryService() override;

  // Queues `event` for upload. Appends timestamp and device identity, then
  // flushes either after the policy-configured batch interval or
  // immediately once the batch reaches its maximum size.
  void RecordPasteEvent(OasisPasteTelemetryEvent event);

  // Returns the policy-configured relay URL, or an invalid GURL when unset
  // or not HTTPS. A command-line override (--oasis-relay-url) is honored
  // for development only.
  GURL GetRelayUrl() const;

  void FlushForTesting();

 private:
  void ScheduleFlush();
  void Flush();
  void OnEnrollmentComplete(bool success);
  void UploadBatch();
  void OnUploadComplete(std::optional<std::string> response_body);

  raw_ptr<Profile> profile_;
  base::Value::List pending_events_;
  base::OneShotTimer flush_timer_;
  std::unique_ptr<network::SimpleURLLoader> upload_loader_;

  base::WeakPtrFactory<OasisTelemetryService> weak_factory_{this};
};

}  // namespace oasis

#endif  // CHROME_BROWSER_OASIS_OASIS_TELEMETRY_SERVICE_H_
