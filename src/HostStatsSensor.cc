/**
 * This file is part of the "sensord" project
 *   Copyright (c) 2015 Paul Asmuth
 *   Copyright (c) 2015 Finn Zirngibl
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <unistd.h>
#include "HostStatsSensor.h"

using namespace fnord;

namespace sensord {

String HostStatsSensor::key() const {
  return "host";
}

void HostStatsSensor::fetchData(HostStats* stats) const {
  stats->mutable_host()->set_hostname(getHostname());

  // -> DiskStats*
  //auto part1 = stats.add_disk_stats();
  //part1->set_mountpoint("/");
  //part1->set_bytes_available(1024 * 1024 * 1024 * 250);
  //part1->set_bytes_used(1024 * 1024 * 1024 * 30);

  //auto part2 = stats.add_disk_stats();
  //part2->set_mountpoint("/boot");
  //part2->set_bytes_available(1024 * 1024 * 512);
  //part2->set_bytes_used(1024 * 1024 * 80);
}

String HostStatsSensor::getHostname() const {
  char buf[1024];

  if (gethostname(buf, sizeof(buf)) != 0) {
    RAISE(kRuntimeError, "gethostname() failed");
  }

  return std::string(buf, strlen(buf));
}

};

