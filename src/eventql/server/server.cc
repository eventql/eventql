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
#include <regex>
#include <eventql/eventql.h>
#include <eventql/config/process_config.h>
#include <eventql/db/database.h>
#include <eventql/server/listener.h>

struct evql_conf_s {
  eventql::ProcessConfigBuilder config_builder;
  std::string error;
};

evql_conf_t* evql_conf_init() {
  auto conf = new evql_conf_s();
  return conf;
}

void evql_conf_free(evql_conf_t* conf)  {
  delete conf;
}

void evql_conf_set(
    evql_conf_t* conf,
    const char* key,
    const char* value) {
  conf->config_builder.setProperty(key, value);
}

int evql_conf_load(
    evql_conf_t* conf,
    const char* fpath) {
  Status rc = Status::success();
  if (fpath) {
    rc = conf->config_builder.loadFile(fpath);
  } else {
    rc = conf->config_builder.loadDefaultConfigFile("evqld");
  }

  return rc.isSuccess() ? 0 : 1;
}

struct evql_server_s {
  eventql::ProcessConfig* config;
  eventql::Database* database;
  std::string error;
};

evql_server_t* evql_server_init(evql_conf_t* conf) {
  auto conf_builder = conf->config_builder;

  auto server = new evql_server_s();
  server->config = conf_builder.getConfig().release();
  server->database = nullptr;
  return server;
}

int evql_server_start(evql_server_t* server) {
  server->database = eventql::Database::newDatabase(server->config);
  auto rc = server->database->start();
  if (rc.isSuccess()) {
    return 0;
  } else {
    server->error = rc.getMessage();
    delete server->database;
    server->database = nullptr;
    return 1;
  }
}

int evql_server_listen(evql_server_t* server, int kill_fd) {
  String listen_host;
  int listen_port;
  {
    auto listen_str = server->config->getString("server.listen");
    if (listen_str.isEmpty()) {
      server->error = "missing 'server.listen' option or --listen flag";
      return 1;
    }

    std::smatch m;
    std::regex listen_regex("([0-9a-zA-Z-_.]+):([0-9]+)");
    if (std::regex_match(listen_str.get(), m, listen_regex)) {
      listen_host = m[1];
      listen_port = std::stoi(m[2]);
    } else {
      server->error = "invalid listen address: " + listen_str.get();
      return 1;
    }
  }

  eventql::Listener listener(server->database);
  auto rc = listener.bind(listen_port);
  if (!rc.isSuccess()) {
    server->error = rc.getMessage();
    return 1;
  }

  listener.run(kill_fd);
  return 0;
}

//int evql_server_handle(evql_server_t* server, int fd, int flags);

void evql_server_shutdown(evql_server_t* server) {
  if (server->database) {
    server->database->shutdown();
    delete server->database;
    server->database = nullptr;
  }
}

void evql_server_free(evql_server_t* server) {
  delete server->config;
  delete server;
}

const char* evql_server_geterror(evql_server_t* server) {
  return server->error.c_str();
}

const char* evql_server_getconf(
    evql_server_t* server,
    const char* key) {
  return server->config->getCString(key);
}

int evql_server_getconfbool(
    evql_server_t* server,
    const char* key) {
  return server->config->getBool(key) ? 1 : 0;
}

