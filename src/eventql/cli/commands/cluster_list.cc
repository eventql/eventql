/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
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
#include <eventql/cli/commands/cluster_list.h>
#include <eventql/cli/cli_config.h>
#include <eventql/sql/result_list.h>

namespace eventql {
namespace cli {

const std::string ClusterList::kName_ = "cluster-list";
const std::string ClusterList::kDescription_ =
    "List the servers of an existing cluster.";
const std::string ClusterList::kQuery_ = "DESCRIBE SERVERS;";

ClusterList::ClusterList(
    RefPtr<ProcessConfig> process_cfg) :
    process_cfg_(process_cfg) {}

Status ClusterList::execute(
    const std::vector<std::string>& argv,
    FileInputStream* stdin_is,
    OutputStream* stdout_os,
    OutputStream* stderr_os) {
  /* cli config */
  eventql::ProcessConfigBuilder cfg_builder;
  cfg_builder.setProperty("client.timeout", "5000000");
  if (!process_cfg_->getString("client.host").isEmpty()) {
    cfg_builder.setProperty(
        "client.host",
        process_cfg_->getString("host").get());
  }

  if (!process_cfg_->getString("client.port").isEmpty()) {
    cfg_builder.setProperty(
        "client.port",
        process_cfg_->getString("client.port").get());
  }

  if (!process_cfg_->getString("client.user").isEmpty()) {
    cfg_builder.setProperty(
        "client.user",
        process_cfg_->getString("client.user").get());
  }

  if (!process_cfg_->getString("client.password").isEmpty()) {
    cfg_builder.setProperty(
        "client.password",
        process_cfg_->getString("client.password").get());
  }

  if (!process_cfg_->getString("client.auth_token").isEmpty()) {
    cfg_builder.setProperty(
        "client.auth_token",
        process_cfg_->getString("client.auth_token").get());
  }

  if (!process_cfg_->getString("client.database").isEmpty()) {
    cfg_builder.setProperty(
        "client.database",
        process_cfg_->getString("client.database").get());
  }

  eventql::cli::CLIConfig cli_cfg(cfg_builder.getConfig());

  auto rc = Status::success();
  auto client = evql_client_init();
  if (!client) {
    rc = Status(eRuntimeError, "can't initialize eventql client");
    goto exit;
  }

  {
    uint64_t timeout = cli_cfg.getTimeout();
    auto ret = evql_client_setopt(
        client,
        EVQL_CLIENT_OPT_TIMEOUT,
        (const char*) &timeout,
        sizeof(timeout),
        0);

    if (ret != 0) {
      rc = Status(eRuntimeError, "can't initialize eventql client");
      goto exit;
    }
  }

  if (!cli_cfg.getUser().isEmpty()) {
    std::string akey = "user";
    std::string aval = cli_cfg.getUser().get();
    evql_client_setauth(
        client,
        akey.data(),
        akey.size(),
        aval.data(),
        aval.size(),
        0);
  }

  if (!cli_cfg.getPassword().isEmpty()) {
    std::string akey = "password";
    std::string aval = cli_cfg.getPassword().get();
    evql_client_setauth(
        client,
        akey.data(),
        akey.size(),
        aval.data(),
        aval.size(),
        0);
  }

  if (!cli_cfg.getAuthToken().isEmpty()) {
    std::string akey = "auth_token";
    std::string aval = cli_cfg.getAuthToken().get();
    evql_client_setauth(
        client,
        akey.data(),
        akey.size(),
        aval.data(),
        aval.size(),
        0);
  }

  {
    std::string database;
    bool switch_database = false;
    if (!cli_cfg.getDatabase().isEmpty()) {
      switch_database = true;
      database = cli_cfg.getDatabase().get();
    }

    auto ret = evql_client_connect(
        client,
        cli_cfg.getHost().c_str(),
        cli_cfg.getPort(),
        switch_database ? database.c_str() : nullptr,
        0);

    if (ret < 0) {
      rc = Status(eRuntimeError, evql_client_geterror(client));
      goto exit;
    }
  }

  {
    auto query_flags = 0;
    int ret = evql_query(client, kQuery_.c_str(), NULL, query_flags);
    size_t result_ncols;
    if (ret == 0) {
      ret = evql_num_columns(client, &result_ncols);
    }

    csql::ResultList results;
    std::vector<std::string> result_columns;
    if (ret < 0) {
      rc = Status(eRuntimeError, evql_client_geterror(client));
      goto exit;
    }

    for (int i = 0; ret == 0 && i < result_ncols; ++i) {
      const char* colname;
      size_t colname_len;
      ret = evql_column_name(client, i, &colname, &colname_len);
      if (ret == -1) {
        break;
      }

      result_columns.emplace_back(colname, colname_len);
    }

    results.addHeader(result_columns);
    size_t result_nrows = 0;
    while (ret >= 0) {
      const char** fields;
      size_t* field_lens;
      ret = evql_fetch_row(client, &fields, &field_lens);
      if (ret < 1) {
        break;
      }

      ++result_nrows;
      std::vector<std::string> row;
      for (int i = 0; i < result_ncols; ++i) {
        row.emplace_back(fields[i], field_lens[i]);
      }

      results.addRow(row);
    }

    results.debugPrint();
    evql_client_releasebuffers(client);
  }

  goto exit;

exit:
  evql_client_destroy(client);
  return rc;
}

const std::string& ClusterList::getName() const {
  return kName_;
}

const std::string& ClusterList::getDescription() const {
  return kDescription_;
}

void ClusterList::printHelp(OutputStream* stdout_os) const {
  stdout_os->write(StringUtil::format(
      "\nevqlctl-$0 - $1\n\n", kName_, kDescription_));

  stdout_os->write(StringUtil::format(
      "Usage: evqlctl $0\n",
      kName_));
}

} // namespace cli
} // namespace eventql

