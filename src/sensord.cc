/**
 * This file is part of the "sensord" project
 *   Copyright (c) 2015 Finn Zirngibl
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "sensord.h"

namespace sensord {

HostStats get_hosts_stats() {
  HostStats stats;
  stats.set_hostname("blaah");
  return stats;
}

};
