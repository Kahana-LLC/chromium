// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/oasis/oasis_telemetry_service.h"

#include <utility>

#include <algorithm>

#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/i18n/time_formatting.h"
#include "base/json/json_writer.h"
#include "base/time/time.h"
#include "chrome/browser/oasis/oasis_device_identity_manager.h"
#include "chrome/browser/oasis/oasis_pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/url_util.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "url/gurl.h"

namespace oasis {

namespace {

// Development-only override for the relay URL; enterprise deployments must
// use the OasisTelemetryRelayUrl policy instead.
constexpr char kOasisRelayUrlSwitch[] = "oasis-relay-url";

constexpr size_t kMaxResponseSize = 16 * 1024;

constexpr net::NetworkTrafficAnnotationTag kTelemetryTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("oasis_paste_telemetry", R"(
        semantics {
          sender: "Oasis Paste Telemetry"
          description:
            "Uploads privacy-reduced metadata about paste events into "
            "monitored LLM sites to the enterprise-configured Oasis relay. "
            "Raw pasted content is never uploaded."
          trigger:
            "A paste into a monitored LLM site while Oasis telemetry is "
            "enabled by enterprise policy."
          data:
            "Provider name, destination origin, coarse content length "
            "bucket, enforcement decision, sensitive-data category names, "
            "device identifier and timestamp."
          destination: OTHER
          destination_other: "Enterprise-configured Oasis relay endpoint."
        }
        policy {
          cookies_allowed: NO
          setting:
            "Controlled by the OasisTelemetryEnabled enterprise policy."
          policy_exception_justification:
            "Disabled unless OasisTelemetryEnabled is set by policy."
        })");

}  // namespace

OasisTelemetryService::OasisTelemetryService(Profile* profile)
    : profile_(profile) {}

OasisTelemetryService::~OasisTelemetryService() = default;

void OasisTelemetryService::RecordPasteEvent(OasisPasteTelemetryEvent event) {
  GURL relay_url = GetRelayUrl();
  if (!relay_url.is_valid()) {
    return;
  }

  base::Value::Dict dict = event.ToDict();
  dict.Set("timestamp", base::TimeFormatAsIso8601(base::Time::Now()));
  dict.Set("device_id", OasisDeviceIdentityManager::GetInstance()->GetDeviceId());
  pending_events_.Append(std::move(dict));

  const PrefService* pref_service = profile_->GetPrefs();
  const int max_events = std::max(
      1, pref_service->GetInteger(prefs::kOasisTelemetryBatchMaxEvents));
  if (static_cast<int>(pending_events_.size()) >= max_events) {
    Flush();
  } else {
    ScheduleFlush();
  }
}

GURL OasisTelemetryService::GetRelayUrl() const {
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  std::string url_string;
  if (command_line->HasSwitch(kOasisRelayUrlSwitch)) {
    url_string = command_line->GetSwitchValueASCII(kOasisRelayUrlSwitch);
  } else {
    url_string =
        profile_->GetPrefs()->GetString(prefs::kOasisTelemetryRelayUrl);
  }

  GURL url(url_string);
  // The relay must terminate TLS with a valid certificate; plain HTTP is
  // only tolerated for local development loops.
  if (!url.is_valid() ||
      (!url.SchemeIs(url::kHttpsScheme) && !net::IsLocalhost(url))) {
    return GURL();
  }
  return url;
}

void OasisTelemetryService::FlushForTesting() {
  Flush();
}

void OasisTelemetryService::ScheduleFlush() {
  if (flush_timer_.IsRunning()) {
    return;
  }
  const int interval_seconds = std::max(
      1, profile_->GetPrefs()->GetInteger(
             prefs::kOasisTelemetryBatchIntervalSeconds));
  flush_timer_.Start(FROM_HERE, base::Seconds(interval_seconds),
                     base::BindOnce(&OasisTelemetryService::Flush,
                                    weak_factory_.GetWeakPtr()));
}

void OasisTelemetryService::Flush() {
  flush_timer_.Stop();
  if (pending_events_.empty() || upload_loader_) {
    return;
  }

  GURL relay_url = GetRelayUrl();
  if (!relay_url.is_valid()) {
    pending_events_.clear();
    return;
  }

  OasisDeviceIdentityManager::GetInstance()->EnsureEnrolled(
      relay_url,
      profile_->GetDefaultStoragePartition()
          ->GetURLLoaderFactoryForBrowserProcess(),
      base::BindOnce(&OasisTelemetryService::OnEnrollmentComplete,
                     weak_factory_.GetWeakPtr()));
}

void OasisTelemetryService::OnEnrollmentComplete(bool success) {
  if (!success) {
    // Keep events queued; the next recorded event re-attempts enrollment.
    ScheduleFlush();
    return;
  }
  UploadBatch();
}

void OasisTelemetryService::UploadBatch() {
  GURL relay_url = GetRelayUrl();
  if (!relay_url.is_valid() || pending_events_.empty()) {
    return;
  }

  base::Value::Dict payload;
  payload.Set("events", std::move(pending_events_));
  pending_events_ = base::Value::List();
  std::string body;
  base::JSONWriter::Write(payload, &body);

  auto request = std::make_unique<network::ResourceRequest>();
  request->url = relay_url.Resolve("/v1/events");
  request->method = "POST";
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->headers.SetHeader(
      "Authorization",
      "Bearer " + OasisDeviceIdentityManager::GetInstance()->GetToken());

  upload_loader_ = network::SimpleURLLoader::Create(
      std::move(request), kTelemetryTrafficAnnotation);
  upload_loader_->AttachStringForUpload(body, "application/json");
  upload_loader_->DownloadToString(
      profile_->GetDefaultStoragePartition()
          ->GetURLLoaderFactoryForBrowserProcess()
          .get(),
      base::BindOnce(&OasisTelemetryService::OnUploadComplete,
                     weak_factory_.GetWeakPtr()),
      kMaxResponseSize);
}

void OasisTelemetryService::OnUploadComplete(
    std::optional<std::string> response_body) {
  std::optional<int> response_code;
  if (upload_loader_->ResponseInfo() &&
      upload_loader_->ResponseInfo()->headers) {
    response_code =
        upload_loader_->ResponseInfo()->headers->response_code();
  }
  upload_loader_.reset();

  // A 401 means the relay revoked our token; re-enroll on the next flush.
  if (response_code == net::HTTP_UNAUTHORIZED) {
    OasisDeviceIdentityManager::GetInstance()->InvalidateToken();
  }
}

}  // namespace oasis
