// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/oasis/oasis_sensitive_data_classifier.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace oasis {

TEST(OasisSensitiveDataClassifierTest, EmptyTextHasNoClassifications) {
  EXPECT_TRUE(ClassifySensitiveData("").empty());
  EXPECT_TRUE(ClassifySensitiveData("just a normal sentence").empty());
}

TEST(OasisSensitiveDataClassifierTest, DetectsAwsAccessKey) {
  auto categories =
      ClassifySensitiveData("key=AKIAIOSFODNN7EXAMPLE region=us-east-1");
  ASSERT_EQ(categories.size(), 1u);
  EXPECT_EQ(categories[0], "aws_access_key");
}

TEST(OasisSensitiveDataClassifierTest, DetectsPrivateKey) {
  auto categories = ClassifySensitiveData(
      "-----BEGIN RSA PRIVATE KEY-----\nMIIEow...\n-----END RSA PRIVATE "
      "KEY-----");
  ASSERT_EQ(categories.size(), 1u);
  EXPECT_EQ(categories[0], "private_key");
}

TEST(OasisSensitiveDataClassifierTest, DetectsSsn) {
  auto categories = ClassifySensitiveData("my ssn is 078-05-1120 thanks");
  ASSERT_EQ(categories.size(), 1u);
  EXPECT_EQ(categories[0], "ssn");
}

TEST(OasisSensitiveDataClassifierTest, DetectsLuhnValidCardOnly) {
  // 4111111111111111 passes Luhn; 4111111111111112 does not.
  EXPECT_EQ(ClassifySensitiveData("card: 4111 1111 1111 1111").size(), 1u);
  EXPECT_TRUE(ClassifySensitiveData("card: 4111 1111 1111 1112").empty());
}

TEST(OasisSensitiveDataClassifierTest, DetectsCredentialAssignment) {
  auto categories = ClassifySensitiveData(
      "api_key = sk_live_abcdefghij1234567890");
  ASSERT_EQ(categories.size(), 1u);
  EXPECT_EQ(categories[0], "credential_assignment");
}

TEST(OasisSensitiveDataClassifierTest, ReportsMultipleCategories) {
  auto categories = ClassifySensitiveData(
      "AKIAIOSFODNN7EXAMPLE and password: hunter2hunter2hunter2");
  EXPECT_EQ(categories.size(), 2u);
}

}  // namespace oasis
