// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OASIS_OASIS_DEVICE_IDENTITY_MANAGER_H_
#define CHROME_BROWSER_OASIS_OASIS_DEVICE_IDENTITY_MANAGER_H_

#include <memory>
#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"

class GURL;

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace oasis {

// Manages this device's identity with the Oasis relay: a stable random
// device ID plus a relay-issued bearer token. Both are stored in local
// state. Tokens are rotated after `kTokenRotationInterval`; the relay can
// revoke a token at any time, in which case uploads fail with 401 and a
// re-enrollment is attempted.
//
// All methods must be called on the UI thread.
class OasisDeviceIdentityManager {
 public:
  using EnrollmentCallback = base::OnceCallback<void(bool success)>;

  static OasisDeviceIdentityManager* GetInstance();

  OasisDeviceIdentityManager(const OasisDeviceIdentityManager&) = delete;
  OasisDeviceIdentityManager& operator=(const OasisDeviceIdentityManager&) =
      delete;

  // Stable device identifier, generated and persisted on first call.
  std::string GetDeviceId();

  // Current relay token; empty until enrollment succeeds.
  std::string GetToken();

  // Ensures the device has a fresh token for `relay_url`. Enrolls if no
  // token exists, rotates if the current token is older than the rotation
  // interval, otherwise invokes `callback` immediately with success.
  void EnsureEnrolled(const GURL& relay_url,
                      scoped_refptr<network::SharedURLLoaderFactory>
                          url_loader_factory,
                      EnrollmentCallback callback);

  // Drops the local token so the next EnsureEnrolled() re-enrolls. Called
  // when the relay rejects the token (revocation).
  void InvalidateToken();

 private:
  friend class base::NoDestructor<OasisDeviceIdentityManager>;

  OasisDeviceIdentityManager();
  ~OasisDeviceIdentityManager();

  void OnEnrollmentResponse(EnrollmentCallback callback,
                            std::optional<std::string> response_body);

  std::unique_ptr<network::SimpleURLLoader> pending_loader_;

  base::WeakPtrFactory<OasisDeviceIdentityManager> weak_factory_{this};
};

}  // namespace oasis

#endif  // CHROME_BROWSER_OASIS_OASIS_DEVICE_IDENTITY_MANAGER_H_
