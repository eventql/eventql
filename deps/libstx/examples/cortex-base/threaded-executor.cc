// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <cortex-base/executor/ThreadedExecutor.h>

int main() {
  cortex::ThreadedExecutor executor;

  executor.execute([]{printf("task 1\n");});
  executor.execute([]{printf("task 2\n");});
  executor.execute([]{printf("task 3\n");});

  executor.joinAll();

  return 0;
}






















