/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#ifndef _STX_TEST_BENCHMARK_H
#define _STX_TEST_BENCHMARK_H
#include <stdlib.h>
#include <stdint.h>
#include "eventql/util/UnixTime.h"

namespace util {

class Benchmark {
public:

  class BenchmarkResult {
  public:
    BenchmarkResult(uint64_t total_time_nanos, uint64_t num_iterations);

    /**
     * Returns the mean time per run over all runs in nanoseconds
     */
    uint64_t meanRuntimeNanos() const;

    /**
     * Returns the number of runs/operations per second
     */
    double ratePerSecond() const;

    /**
     * How many times did we run the benchmark?
     */
    uint64_t numIterations() const;

  protected:
    const uint64_t total_time_nanos_;
    const uint64_t num_iterations_;
  };

  /**
   * Execute the provided function N times and return the measured timing data
   *
   * @param subject the function to execute
   * @param num_iterations how often to execute the benchmark
   */
  static BenchmarkResult benchmark(
      std::function<void()> subject,
      uint64_t num_iterations = 1000);

  /**
   * Execute the provided function N * M times and print the measured timing
   * data as a table
   *
   * @param subject the function to execute
   * @param num_iterations how often to execute the benchmark per round
   * @param num_rounds how many rounds of benchmark to execute
   */
  static void benchmarkAndPrint(
      std::function<void()> subject,
      uint64_t num_iterations = 1000,
      uint64_t num_rounds = 10);

  /**
   * Print a benchmark result as a table
   */
  static void printResultTable(
    const std::string& label,
    const BenchmarkResult& result,
    bool append = false);

};

}
#endif
