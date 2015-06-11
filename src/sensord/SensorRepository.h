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
#include <fnord/stdtypes.h>
#include <fnord/autoref.h>
#include "Sensor.h"

using namespace fnord;

namespace sensord {

class SensorRepository {
public:

  void addSensor(RefPtr<Sensor> sensor);

  RefPtr<Sensor> fetchSensor(const String& key) const;

  BufferRef fetchSensorData(const String& key) const;

  template <typename T>
  T fetchSensorDataAs(const String& key) const;

  size_t numSensors() const;

protected:
  mutable std::mutex mutex_;
  HashMap<String, RefPtr<Sensor>> sensors_;
};

};

#include "SensorRepository_impl.h"
