// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <cortex-base/io/GzipFilter.h>
#include <cortex-base/Buffer.h>
#include <unistd.h>
#include <stdio.h>

using namespace cortex;

int main(int argc, const char* argv[]) {
  GzipFilter gzip(9);
  Buffer input(4096);
  Buffer output(4096);

  while (!feof(stdin)) {
    ssize_t n = fread(input.data(), 1, input.capacity(), stdin);
    if (n < 0) {
      perror("fread");
      return 1;
    }
    input.resize(n);
    fprintf(stderr, "read %zi bytes\n", n);
    gzip.filter(input, &output, false);
    fprintf(stderr, "compressed to %zu bytes\n", output.size());
    fwrite(output.data(), 1, output.size(), stdout);
    output.clear();
  }
  gzip.filter(BufferRef(), &output, true);
  fprintf(stderr, "compressed to %zu bytes (finalize)\n", output.size());
  fwrite(output.data(), output.size(), 1, stdout);
  fflush(stdout);

  return 0;
}
