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
#pragma once
#include "fnord-base/stdtypes.h"
#include "HostStats.pb.h"

using namespace fnord;

namespace sensord {

class HostStatsSensor : public ProtoSensor<HostStats> {
public:
  HostStatsSensor();
  void fetchData(HostStats* stats) const override;
};

};

