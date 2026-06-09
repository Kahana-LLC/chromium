// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/oasis/oasis_device_identity_manager.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/time/time.h"
#include "base/uuid.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/oasis/oasis_pref_names.h"
#include "components/prefs/pref_service.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/gurl.h"

namespace oasis {

namespace {

// Tokens are rotated after this interval even if still accepted.
constexpr base::TimeDelta kTokenRotationInterval = base::Days(30);

constexpr size_t kMaxResponseSize = 16 * 1024;

constexpr net::NetworkTrafficAnnotationTag kEnrollmentTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("oasis_device_enrollment", R"(
        semantics {
          sender: "Oasis Device Identity"
          description:
            "Enrolls this browser installation with the Oasis telemetry "
            "relay and rotates the relay-issued device token."
          trigger:
            "First telemetry upload after Oasis telemetry is enabled by "
            "enterprise policy, or when the device token is due for "
            "rotation or has been revoked."
          data:
            "A random device identifier. No user or page content."
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

PrefService* LocalState() {
  return g_browser_process->local_state();
}

}  // namespace

// static
OasisDeviceIdentityManager* OasisDeviceIdentityManager::GetInstance() {
  static base::NoDestructor<OasisDeviceIdentityManager> instance;
  return instance.get();
}

OasisDeviceIdentityManager::OasisDeviceIdentityManager() = default;
OasisDeviceIdentityManager::~OasisDeviceIdentityManager() = default;

std::string OasisDeviceIdentityManager::GetDeviceId() {
  PrefService* local_state = LocalState();
  std::string device_id = local_state->GetString(prefs::kOasisDeviceId);
  if (device_id.empty()) {
    device_id = base::Uuid::GenerateRandomV4().AsLowercaseString();
    local_state->SetString(prefs::kOasisDeviceId, device_id);
  }
  return device_id;
}

std::string OasisDeviceIdentityManager::GetToken() {
  return LocalState()->GetString(prefs::kOasisDeviceToken);
}

void OasisDeviceIdentityManager::EnsureEnrolled(
    const GURL& relay_url,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    EnrollmentCallback callback) {
  PrefService* local_state = LocalState();
  const std::string token = GetToken();
  const base::Time rotated_at =
      local_state->GetTime(prefs::kOasisDeviceTokenRotatedAt);
  const bool needs_rotation =
      !token.empty() &&
      base::Time::Now() - rotated_at > kTokenRotationInterval;

  if (!token.empty() && !needs_rotation) {
    std::move(callback).Run(true);
    return;
  }

  if (pending_loader_) {
    // An enrollment is already in flight; report failure so the caller
    // retries on the next batch.
    std::move(callback).Run(false);
    return;
  }

  auto request = std::make_unique<network::ResourceRequest>();
  request->url =
      relay_url.Resolve(needs_rotation ? "/v1/rotate" : "/v1/enroll");
  request->method = "POST";
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  if (needs_rotation) {
    request->headers.SetHeader("Authorization", "Bearer " + token);
  }

  base::Value::Dict body;
  body.Set("device_id", GetDeviceId());
  std::string body_json;
  base::JSONWriter::Write(body, &body_json);

  pending_loader_ = network::SimpleURLLoader::Create(
      std::move(request), kEnrollmentTrafficAnnotation);
  pending_loader_->AttachStringForUpload(body_json, "application/json");
  pending_loader_->DownloadToString(
      url_loader_factory.get(),
      base::BindOnce(&OasisDeviceIdentityManager::OnEnrollmentResponse,
                     weak_factory_.GetWeakPtr(), std::move(callback)),
      kMaxResponseSize);
}

void OasisDeviceIdentityManager::InvalidateToken() {
  PrefService* local_state = LocalState();
  local_state->ClearPref(prefs::kOasisDeviceToken);
  local_state->ClearPref(prefs::kOasisDeviceTokenRotatedAt);
}

void OasisDeviceIdentityManager::OnEnrollmentResponse(
    EnrollmentCallback callback,
    std::optional<std::string> response_body) {
  pending_loader_.reset();

  if (!response_body) {
    std::move(callback).Run(false);
    return;
  }

  std::optional<base::Value::Dict> response = base::JSONReader::ReadDict(
      *response_body, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!response) {
    std::move(callback).Run(false);
    return;
  }
  const std::string* new_token = response->FindString("token");
  if (!new_token || new_token->empty()) {
    std::move(callback).Run(false);
    return;
  }

  PrefService* local_state = LocalState();
  local_state->SetString(prefs::kOasisDeviceToken, *new_token);
  local_state->SetTime(prefs::kOasisDeviceTokenRotatedAt, base::Time::Now());
  std::move(callback).Run(true);
}

}  // namespace oasis
