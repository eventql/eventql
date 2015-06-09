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
#include <fnord-base/stdtypes.h>
#include <fnord-base/autoref.h>
#include <fnord-base/thread/TaskScheduler.h>
#include <sensord/SampleEnvelope.pb.h>

using namespace fnord;

namespace sensord {

class SensorSampleFeed {
public:
  typedef Function<void (const SampleEnvelope& sample)> CallbackFn;
  typedef Function<bool (const SampleEnvelope& sample)> PredicateFn;
  typedef size_t SubscriptionID;

  SubscriptionID subscribe(
      TaskScheduler* scheduler,
      CallbackFn callback);

  SubscriptionID subscribe(
      TaskScheduler* scheduler,
      CallbackFn callback,
      PredicateFn predicate);

  void unsubscribe(SubscriptionID id);

  void publish(SampleEnvelope sample);

protected:
  struct PublishedSample : public RefCounted {
    const PublishedSample(const SampleEnvelope& s) : sample(s) {}
    SampleEnvelope sample;
  };

  struct Subscription {
    CallbackFn callback;
    PredicateFn predicate;
    TaskScheduler* scheduler;
  };

  mutable std::mutex mutex_;
  Vector<Subscription> subscriptions_;
};

};

