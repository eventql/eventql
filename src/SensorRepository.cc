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
#include "SensorRepository.h"

using namespace fnord;

namespace sensord {

void SensorRepository::addSensor(RefPtr<Sensor> sensor) {
  std::unique_lock<std::mutex> lk(mutex_);
  auto key = sensor->key();

  if (sensors_.count(key) > 0) {
    RAISEF(
        kIllegalStateError,
        "sensor with key '$0' already registered",
        key);
  }

  sensors_.emplace(key, sensor);
}


RefPtr<Sensor> SensorRepository::fetchSensor(const String& key) const {
  std::unique_lock<std::mutex> lk(mutex_);

  auto iter = sensors_.find(key);
  if (iter == sensors_.end()) {
    RAISEF(kIndexError, "sensor not found: '$0'", key);
  }

  return iter->second;
}

BufferRef SensorRepository::fetchSensorData(const String& key) const {
  auto sensor = fetchSensor(key);
  return sensor->fetchData();
}

};

