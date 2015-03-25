/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "ReportBuilder.h"
#include <thread>

using namespace fnord;

namespace cm {

ReportBuilder::ReportBuilder() : max_threads_(8) {}

void ReportBuilder::addReport(RefPtr<Report> report) {
  reports_.emplace_back(report);
}

void ReportBuilder::buildAll() {
  while (buildOnce() > 0) {}
}

size_t ReportBuilder::buildOnce() {
  Set<String> existing_files;
  fnord::logInfo("cm.reportbuild", "Scanning dependencies...");

  uint64_t reports_waiting = 0;
  uint64_t reports_runnable = 0;
  uint64_t reports_completed = 0;

  HashMap<ReportSource*, List<RefPtr<Report>>> runnables;
  for (const auto& r : reports_) {
    bool inputs_ready = true;
    for (const auto& f : r->input()->inputFiles()) {
      if (existing_files.count(f) > 0 || FileUtil::exists(f)) {
        existing_files.emplace(f);
      } else {
        fnord::logDebug(
            "cm.reportbuild",
            "Marking report '$0' as waiting because input '$1' doesn't exist",
            "FIXME",
            f);

        inputs_ready = false;
        break;
      }
    }

    if (!inputs_ready) {
      ++reports_waiting;
      continue;
    }

    bool already_completed = true;
    for (const auto& f : r->output()->outputFiles()) {
      if (existing_files.count(f) > 0 || FileUtil::exists(f)) {
        existing_files.emplace(f);
      } else {
        fnord::logDebug(
            "cm.reportbuild",
            "Marking report '$0' as runnable because output '$1' doesn't exist",
            "FIXME",
            f);

        already_completed = false;
        break;
      }
    }

    if (already_completed) {
      ++reports_completed;
    } else {
      ++reports_runnable;
      runnables[r->input().get()].emplace_back(r);
    }
  }

  fnord::logInfo(
      "cm.reportbuild",
      "Found $0 report(s) - $1 waiting, $2 runnable, $3 completed",
      reports_waiting + reports_runnable + reports_completed,
      reports_waiting,
      reports_runnable,
      reports_completed);

  fnord::logInfo(
      "cm.reportbuild",
      "Running $0 report pipeline(s)", runnables.size());

  List<std::thread> threads;
  for (auto runnable : runnables) {
    threads.emplace_back([runnable] () {
      fnord::logInfo(
          "cm.reportbuild",
          "Running report pipline with $1 report(s) for inputs: $0",
          inspect(runnable.first->inputFiles()),
          runnable.second.size());

      for (const auto& r : runnable.second) {
        r->onInit();
      }

      runnable.first->read();

      for (const auto& r : runnable.second) {
        r->onFinish();
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  return reports_runnable;
}

} // namespace cm

