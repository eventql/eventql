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
#include <eventql/eventql.h>
#include <eventql/config/process_config.h>

struct evql_server_s {
  eventql::ProcessConfigBuilder* config_builder;
  eventql::ProcessConfig* config;
};

evql_server_t* evql_server_init() {
  auto server = static_cast<evql_server_t*>(malloc(sizeof(evql_server_t)));
  if (!server) {
    return nullptr;
  }

  server->config_builder = new eventql::ProcessConfigBuilder();
  server->config = nullptr;
  return server;
}

//
//void evql_server_cfgset(
//    evql_server_t* server,
//    const char* key,
//    const char* value);
//
//int evql_server_start(evql_server_t* server);
//int evql_server_listen(evql_server_t* server, int kill_fd);
//int evql_server_handle(evql_server_t* server, int fd, int flags);
//void evql_server_shutdown(evql_server_t* server);
//void evql_server_free(evql_server_t* server);
//const char* evql_server_geterror(evql_server_t* server);
//

//  //Database db(process_config.get());
//  //auto rc = db.start();
//
//  /* listen addr */
//  String listen_host;
//  int listen_port;
//  {
//    auto listen_str = process_config->getString("server.listen");
//    if (listen_str.isEmpty()) {
//      logFatal("evqld", "missing 'server.listen' option or --listen flag");
//      return 1;
//    }
//
//    std::smatch m;
//    std::regex listen_regex("([0-9a-zA-Z-_.]+):([0-9]+)");
//    if (std::regex_match(listen_str.get(), m, listen_regex)) {
//      listen_host = m[1];
//      listen_port = std::stoi(m[2]);
//    } else {
//      logFatal("evqld", "invalid listen address: $0", listen_str.get());
//      return 1;
//    }
//  }
//
//
//  ///* start listener */
//  //Listener listener(nullptr);
//  //if (rc.isSuccess()) {
//  //  rc = listener.bind(listen_port);
//  //}
//
//  //if (rc.isSuccess()) {
//  //  listener.run();
//  //}

