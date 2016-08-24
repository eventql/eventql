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
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <regex>
#include <sys/time.h>
#include <sys/resource.h>
#include "eventql/eventql.h"
#include "eventql/util/application.h"
#include "eventql/util/logging.h"
#include "eventql/util/io/fileutil.h"

evql_conf_t* server_conf;
evql_server_t* server;

void testserver_start() {
  server_conf = evql_conf_init();
  if (!server_conf) {
    std::cerr << "FAIL: smoketest - can't start testserver";
    exit(1);
  }

  evql_conf_set(server_conf, "server.datadir", "/tmp/evql-smoketest-1");
  evql_conf_set(server_conf, "server.indexbuild_threads", "1");
  evql_conf_set(server_conf, "server.gc_mode", "AUTOMATIC");
  evql_conf_set(server_conf, "server.gc_interval", "30000000");
  evql_conf_set(server_conf, "server.cachedir_maxsize", "68719476736");
  evql_conf_set(server_conf, "server.noleader", "true");
  evql_conf_set(server_conf, "server.name", "testserver");
  evql_conf_set(server_conf, "server.client_auth_backend", "trust");
  evql_conf_set(server_conf, "server.noleader", "true");
  evql_conf_set(server_conf, "cluster.coordinator", "standalone");

  server = evql_server_init(server_conf);
  if (!server) {
    std::cerr << "FAIL: smoketest - can't start testserver";
    exit(1);
  }

  if (!evql_server_start(server)) {
    std::cerr
        << "FAIL: smoketest - can't start testserver: "
        << evql_server_geterror(server);

    exit(1);
  }
}

void testserver_stop() {
  evql_server_shutdown(server);
  evql_server_free(server);
  evql_conf_free(server_conf);
}

int main(int argc, const char** argv) {
  testserver_start();

  testserver_stop();
  return 0;
}

