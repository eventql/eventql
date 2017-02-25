/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
#include <string>
#include <string.h>

#include "eventql/util/exception.h"
#include "eventql/util/inspect.h"
#include "eventql/util/random.h"
#include "eventql/util/io/fileutil.h"
#include "eventql/util/io/inputstream.h"
#include "eventql/util/io/outputstream.h"
#include "test_repository.h"

const char kExpectationFailed[] = "ExpectationFailed";

#define EXPECT(X) \
    if (!(X)) { \
      RAISE( \
          kExpectationFailed, \
          "expectation failed: %s", #X); \
    }

#define EXPECT_TRUE(X) \
  if (!(X)) { \
    RAISE( \
        kExpectationFailed, \
        "expectation failed: expected TRUE, got FALSE"); \
  }

#define EXPECT_FALSE(X) \
  if (!!(X)) { \
    RAISE( \
        kExpectationFailed, \
        "expectation failed: expected FALSE, got TRUE"); \
  }

template <typename T1, typename T2, typename T3>
void EXPECT_NEAR(T1 expected, T2 actual, T3 diff) {
  if (actual < (expected - diff) || actual > (expected + diff)) {
    RAISE(
        kExpectationFailed,
        "expectation failed: actual %s near expected %s, diff %s",
        inspect<T2>(actual).c_str(),
        inspect<T1>(expected).c_str(),
        inspect<T3>(diff).c_str());
  }
}

template <typename T1, typename T2>
void EXPECT_EQ(T1 left, T2 right) {
  if (!(left == right)) {
    RAISE(
        kExpectationFailed,
        "expectation failed: %s == %s",
        inspect<T1>(left).c_str(),
        inspect<T2>(right).c_str());
  }
}

#define EXPECT_EXCEPTION(E, L) \
    { \
      bool raised = false; \
      try { \
        L(); \
      } catch (const std::exception& e) { \
        raised = true; \
        if (strcmp(e.what(), E) != 0) { \
          RAISE( \
              kExpectationFailed, \
              "excepted exception '%s' but got '%s'", E, e.what()); \
        } \
      } \
      if (!raised) { \
        RAISE( \
            kExpectationFailed, \
            "excepted exception '%s' but got no exception", E); \
      } \
    }

#define EXPECT_FILES_EQ(F1, F2) \
  { \
    auto one = FileInputStream::openFile(F1); \
    auto two = FileInputStream::openFile(F2); \
    std::string one_str; \
    std::string two_str; \
    one->readUntilEOF(&one_str); \
    two->readUntilEOF(&two_str); \
    if (one_str != two_str) { \
      std::string filename1(F1); \
      std::string filename2(F2); \
      RAISE( \
          kExpectationFailed, \
          "expected files '%s' and '%s' to be equal, but the differ", \
          filename1.c_str(), filename2.c_str()); \
    } \
  }

#define SETUP_UNIT_TESTCASE(CASES, ID, UNIT, TEST) do { \
    (CASES)->emplace_back(eventql::test::TestCase { \
      .test_id = (ID), \
      .description = "Test " + std::string(#TEST) + " in " + std::string(#UNIT), \
      .fun = &test_##UNIT##_##TEST, \
      .suites =  { TestSuite::WORLD, TestSuite::SMOKE } \
    }); \
  } while (0)

#define SETUP_UNIT_TEST(T, R) do { \
    extern void setup_unit_##T##_tests(TestRepository* repo); \
    setup_unit_##T##_tests((R)); \
  } while (0)

