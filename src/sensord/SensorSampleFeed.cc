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
#include <fnord/exception.h>
#include <sensord/SensorSampleFeed.h>

using namespace fnord;

namespace sensord {

SensorSampleFeed::SubscriptionID SensorSampleFeed::subscribe(
    TaskScheduler* scheduler,
    CallbackFn callback) {
  return subscribe(
    scheduler,
    callback,
    [] (const SampleEnvelope& smpl) { return true; });
}

SensorSampleFeed::SubscriptionID SensorSampleFeed::subscribe(
    TaskScheduler* scheduler,
    CallbackFn callback,
    PredicateFn predicate) {
  std::unique_lock<std::mutex> lk(mutex_);

  subscriptions_.emplace_back(Subscription {
    .callback = callback,
    .predicate = predicate,
    .scheduler = scheduler
  });

  return subscriptions_.size() - 1;
}

void SensorSampleFeed::unsubscribe(SubscriptionID id) {
  RAISE(kNotYetImplementedError);
}

void SensorSampleFeed::publish(SampleEnvelope sample) {
  std::unique_lock<std::mutex> lk(mutex_);

  auto published = mkRef(new PublishedSample(sample));
  for (const auto& sub : subscriptions_) {
    if (!sub.predicate(sample)) {
      continue;
    }

    auto cb = sub.callback;
    sub.scheduler->run([published, cb] {
      cb(published->sample);
    });
  }
}

};

