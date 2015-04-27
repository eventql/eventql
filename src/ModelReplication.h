/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_MODELREPLICATION_H
#define _CM_MODELREPLICATION_H
#include <mutex>
#include <stdlib.h>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <fnord-base/autoref.h>

using namespace fnord;

namespace cm {

class ModelReplication {
public:

  ModelReplication();

  void addJob(const String& name, Function<void()> fn);

  void start();
  void stop();

protected:
  void run();

  Duration interval_;
  bool running_;
  std::thread thread_;
  List<Pair<String, Function<void()>>> jobs_;
};

} // namespace cm

#endif
