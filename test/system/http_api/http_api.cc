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
#include <unistd.h>
#include "http_api.h"
#include "../../automate/cluster.h"
#include "../../automate/curl.h"
#include "../../test_runner.h"

namespace eventql {
namespace test {
namespace system_http_api {

static void setup_tests(TestBundle* test_bundle, const std::string& id_prefix) {
  {
    TestCase t;
    t.test_id = id_prefix + "002";
    t.description = "Execute sql format=json";
    t.suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE };
    t.fun = [] (TestContext* ctx) -> bool {
      executePOST(
          ctx,
          "test/system/http_api/execute_sql_json",
          "sql",
          "localhost",
          "9175",
          "{\"database\": \"test\", \"query\": \"SELECT 1;\"}");

      return true;
    };
    test_bundle->test_cases.emplace_back(t);
  }

  {
    TestCase t;
    t.test_id = id_prefix + "003";
    t.description = "Execute sql format=json_sse";
    t.suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE };
    t.fun = [] (TestContext* ctx) -> bool {
      std::string body = "{ \
          \"database\": \"test\", \
          \"query\": \"SELECT 1;\", \
          \"format\": \"json_sse\" \
        }";

      executePOST(
          ctx,
          "test/system/http_api/execute_sql_jsonsse",
          "sql",
          "localhost",
          "9175",
          body);

      return true;
    };
    test_bundle->test_cases.emplace_back(t);
  }

  {
    TestCase t;
    t.test_id = id_prefix + "004";
    t.description = "Create Table";
    t.suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE };
    t.fun = [] (TestContext* ctx) -> bool {
      std::string body =
          "{\"database\": \"test\", \
            \"table_name\": \"pages\", \
             \"primary_key\": [\"id\"], \
             \"columns\": [{\
                \"name\": \"id\", \"type\": \"UINT64\", \
                \"name\": \"path\", \"type\": \"STRING\" \
            }] \
          }";

      executePOST(
          ctx,
          "test/system/http_api/create_table",
          "tables/create",
          "localhost",
          "9175",
          body);

      return true;
    };
    test_bundle->test_cases.emplace_back(t);
  }

  {
    TestCase t;
    t.test_id = id_prefix + "005";
    t.description = "List Tables";
    t.suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE };
    t.fun = [] (TestContext* ctx) -> bool {
      executePOST(
          ctx,
          "test/system/http_api/list_tables",
          "tables/list",
          "localhost",
          "9175",
          "{\"database\": \"test\"}");

      return true;
    };
    test_bundle->test_cases.emplace_back(t);
  }

  {
    TestCase t;
    t.test_id = id_prefix + "006";
    t.description = "Add table field";
    t.suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE };
    t.fun = [] (TestContext* ctx) -> bool {
      std::string body = "{ \
          \"database\": \"test\", \
          \"table\": \"pages\", \
          \"field_name\": \"title\", \
          \"field_type\": \"STRING\", \
          \"repeated\": \"false\", \
          \"optional\": \"true\" \
        }";

      executePOST(
          ctx,
          "test/system/http_api/add_table_field",
          "tables/add_field",
          "localhost",
          "9175",
          body);

      return true;
    };
    test_bundle->test_cases.emplace_back(t);
  }

  {
    TestCase t;
    t.test_id = id_prefix + "007";
    t.description = "Remove table field";
    t.suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE };
    t.fun = [] (TestContext* ctx) -> bool {
      std::string body = "{ \
          \"database\": \"test\", \
          \"table\": \"pages\", \
          \"field_name\": \"title\" \
        }";

      executePOST(
          ctx,
          "test/system/http_api/remove_table_field",
          "tables/remove_field",
          "localhost",
          "9175",
          body);

      return true;
    };
    test_bundle->test_cases.emplace_back(t);
  }

  {
    TestCase t;
    t.test_id = id_prefix + "008";
    t.description = "Insert";
    t.suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE };
    t.fun = [] (TestContext* ctx) -> bool {
      std::string body = "[{ \
          \"table\": \"pages\", \
          \"database\": \"test\", \
          \"data\": {\"id\": 123} \
        }]";

      executePOST(
          ctx,
          "test/system/http_api/insert",
          "tables/insert",
          "localhost",
          "9175",
          body);

      return true;
    };
    test_bundle->test_cases.emplace_back(t);
  }

  {
    TestCase t;
    t.test_id = id_prefix + "009";
    t.description = "Describe table";
    t.suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE };
    t.fun = [] (TestContext* ctx) -> bool {
      executePOST(
          ctx,
          "test/system/http_api/describe_table",
          "tables/describe",
          "localhost",
          "9175",
          "{\"database\": \"test\", \"table\": \"pages\"}");

      return true;
    };
    test_bundle->test_cases.emplace_back(t);
  }

  {
    TestCase t;
    t.test_id = id_prefix + "010";
    t.description = "Drop table";
    t.suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE };
    t.fun = [] (TestContext* ctx) -> bool {
      executePOST(
          ctx,
          "test/system/http_api/drop_table",
          "tables/drop",
          "localhost",
          "9175",
          "{\"database\": \"test\", \"table\": \"pages\"}");

      return true;
    };
    test_bundle->test_cases.emplace_back(t);
  }
}

void setup_tests(TestRepository* test_repo) {
  // standalone cluster
  {
    TestBundle t;
    t.logfile_path = "test/system/http_api/test.log";

    TestCase tc;
    tc.test_id = "SYS-HTTPAPI-STANDALONE-001";
    tc.description = "start";
    tc.suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE };
    tc.fun = [] (TestContext* ctx) -> bool {
      startStandaloneCluster(ctx);

      return true;
    };
    t.test_cases.emplace_back(tc);

    setup_tests(&t, "SYS-HTTPAPI-STANDALONE-");
    test_repo->addTestBundle(t);
  }
}

} // namespace system_http_api
} // namespace test
} // namespace eventql

