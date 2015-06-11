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
#include "fnord/stdtypes.h"
#include "fnord/autoref.h"
#include "fnord/protobuf/MessageSchema.h"

using namespace fnord;

namespace sensord {

class Sensor : public RefCounted {
public:

  virtual ~Sensor() {};

  /**
   * The key under which this sensor will be available. e.g. "host/load_average"
   */
  virtual String key() const = 0;

  /**
   * Fetch this sensors most latest data snapshot
   */
  virtual BufferRef fetchData() const = 0;

  /**
   * Return the schema for the data snapshots returned by this sensor
   */
  virtual RefPtr<msg::MessageSchema> schema() const = 0;

  /**
   * Return the name of the schema for the data returned by this sensor.
   * e.g "sensord.HostStats"
   */
  const String& schemaName() const;

};

template <typename ProtoType>
class ProtoSensor : public Sensor {
public:

  ProtoSensor();

  BufferRef fetchData() const override;
  virtual void fetchData(ProtoType*) const = 0;

  RefPtr<msg::MessageSchema> schema() const override;

protected:
  String key_;
  RefPtr<msg::MessageSchema> schema_;
};

};

#include "Sensor_impl.h"
