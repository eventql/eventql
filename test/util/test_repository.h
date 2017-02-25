/**
 * Copyright (c) 2017 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
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
#include <iostream>
#include <functional>
#include <set>
#include <vector>
#include <list>

namespace eventql {
namespace test {

enum class TestSuite {
  WORLD,
  SMOKE
};

struct TestCase {
  std::string test_id;
  std::function<bool ()> fun;
  std::set<TestSuite> suites;
};

class TestRepository {
public:

  TestRepository();

  void addTestBundle(std::vector<TestCase> test_bundle);

  const std::list<std::vector<TestCase>>& getTestBundles() const;
  size_t getTestCount() const;

protected:
  std::list<std::vector<TestCase>> test_bundles_;
  size_t test_count_;
};

} // namespace test
} // namespace eventql

