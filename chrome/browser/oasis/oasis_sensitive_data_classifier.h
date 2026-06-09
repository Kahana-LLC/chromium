// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OASIS_OASIS_SENSITIVE_DATA_CLASSIFIER_H_
#define CHROME_BROWSER_OASIS_OASIS_SENSITIVE_DATA_CLASSIFIER_H_

#include <string>
#include <string_view>
#include <vector>

namespace oasis {

// Lightweight on-device classification of pasted text. Runs in the browser
// process, before the content is delivered to the page. Only category names
// are reported in telemetry; the matched text never leaves the device.
//
// Returned categories (deduplicated):
//   "aws_access_key", "private_key", "jwt", "ssn", "credit_card",
//   "credential_assignment"
std::vector<std::string> ClassifySensitiveData(std::string_view utf8_text);

}  // namespace oasis

#endif  // CHROME_BROWSER_OASIS_OASIS_SENSITIVE_DATA_CLASSIFIER_H_
