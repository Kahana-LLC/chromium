// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/oasis/oasis_sensitive_data_classifier.h"

#include "base/no_destructor.h"
#include "third_party/re2/src/re2/re2.h"

namespace oasis {

namespace {

// Validates a candidate card number with the Luhn checksum to keep the
// false-positive rate of the broad digit-run regex acceptable.
bool PassesLuhn(std::string_view digits) {
  if (digits.size() < 13 || digits.size() > 19) {
    return false;
  }
  int sum = 0;
  bool doubled = false;
  for (size_t i = digits.size(); i > 0; --i) {
    int d = digits[i - 1] - '0';
    if (doubled) {
      d *= 2;
      if (d > 9) {
        d -= 9;
      }
    }
    sum += d;
    doubled = !doubled;
  }
  return sum % 10 == 0;
}

bool ContainsLuhnValidCardNumber(std::string_view text) {
  static const base::NoDestructor<RE2> kCardCandidate(
      R"(\b((?:\d[ -]?){12,18}\d)\b)");
  std::string_view input = text;
  std::string candidate;
  while (RE2::FindAndConsume(&input, *kCardCandidate, &candidate)) {
    std::string digits;
    digits.reserve(candidate.size());
    for (char c : candidate) {
      if (c >= '0' && c <= '9') {
        digits.push_back(c);
      }
    }
    if (PassesLuhn(digits)) {
      return true;
    }
  }
  return false;
}

}  // namespace

std::vector<std::string> ClassifySensitiveData(std::string_view utf8_text) {
  std::vector<std::string> categories;
  if (utf8_text.empty()) {
    return categories;
  }

  static const base::NoDestructor<RE2> kAwsAccessKey(
      R"(\bAKIA[0-9A-Z]{16}\b)");
  static const base::NoDestructor<RE2> kPrivateKey(
      R"(-----BEGIN [A-Z ]*PRIVATE KEY-----)");
  static const base::NoDestructor<RE2> kJwt(
      R"(\beyJ[A-Za-z0-9_-]{10,}\.[A-Za-z0-9_-]{10,}\.[A-Za-z0-9_-]{10,})");
  static const base::NoDestructor<RE2> kSsn(R"(\b\d{3}-\d{2}-\d{4}\b)");
  static const base::NoDestructor<RE2> kCredentialAssignment(
      R"((?i)\b(?:api[_-]?key|secret|token|passwd|password)\b\s*[:=]\s*['"]?[A-Za-z0-9_\-./+]{16,})");

  if (RE2::PartialMatch(utf8_text, *kAwsAccessKey)) {
    categories.push_back("aws_access_key");
  }
  if (RE2::PartialMatch(utf8_text, *kPrivateKey)) {
    categories.push_back("private_key");
  }
  if (RE2::PartialMatch(utf8_text, *kJwt)) {
    categories.push_back("jwt");
  }
  if (RE2::PartialMatch(utf8_text, *kSsn)) {
    categories.push_back("ssn");
  }
  if (ContainsLuhnValidCardNumber(utf8_text)) {
    categories.push_back("credit_card");
  }
  if (RE2::PartialMatch(utf8_text, *kCredentialAssignment)) {
    categories.push_back("credential_assignment");
  }

  return categories;
}

}  // namespace oasis
