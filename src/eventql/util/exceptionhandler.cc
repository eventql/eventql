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
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <eventql/util/exception.h>
#include <eventql/util/exceptionhandler.h>
#include <eventql/util/inspect.h>
#include <eventql/util/logging.h>

CatchAndLogExceptionHandler::CatchAndLogExceptionHandler(
    const String& component) :
    component_(component) {}

void CatchAndLogExceptionHandler::onException(
    const std::exception& error) const {
  logError(component_, error, "Uncaught exception");
}

CatchAndAbortExceptionHandler::CatchAndAbortExceptionHandler(
    const std::string& message) :
    message_(message) {}

void CatchAndAbortExceptionHandler::onException(
    const std::exception& error) const {
  fprintf(stderr, "%s\n\n", message_.c_str()); // FIXPAUL

  try {
    auto rte = dynamic_cast<const Exception&>(error);
    rte.debugPrint();
  } catch (const std::exception& cast_error) {
    fprintf(stderr, "foreign exception: %s\n", error.what());
  }

  fprintf(stderr, "Aborting...\n");
  abort(); // core dump if enabled
}

static std::string globalEHandlerMessage;

static void globalSEGVHandler(int sig, siginfo_t* siginfo, void* ctx) {
  fprintf(stderr, "%s\n", globalEHandlerMessage.c_str());
  fprintf(stderr, "signal: %s\n", strsignal(sig));

  exit(EXIT_FAILURE);
}

static void globalEHandler() {
  fprintf(stderr, "%s\n", globalEHandlerMessage.c_str());

  auto ex = std::current_exception();
  if (ex == nullptr) {
    fprintf(stderr, "<no active exception>\n");
    return;
  }

  try {
    std::rethrow_exception(ex);
  } catch (const Exception& e) {
    e.debugPrint();
  } catch (const std::exception& e) {
    fprintf(stderr, "foreign exception: %s\n", e.what());
  }

  exit(EXIT_FAILURE);
}

void CatchAndAbortExceptionHandler::installGlobalHandlers() {
  globalEHandlerMessage = message_;
  std::set_terminate(&globalEHandler);
  std::set_unexpected(&globalEHandler);

  //struct sigaction sigact;
  //memset(&sigact, 0, sizeof(sigact));
  //sigact.sa_sigaction = &globalSEGVHandler;
  //sigact.sa_flags = SA_SIGINFO;

  //if (sigaction(SIGSEGV, &sigact, NULL) < 0) {
  //  RAISE_ERRNO(kIOError, "sigaction() failed");
  //}

  //if (sigaction(SIGABRT, &sigact, NULL) < 0) {
  //  RAISE_ERRNO(kIOError, "sigaction() failed");
  //}

  //if (sigaction(SIGBUS, &sigact, NULL) < 0) {
  //  RAISE_ERRNO(kIOError, "sigaction() failed");
  //}

  //if (sigaction(SIGSYS, &sigact, NULL) < 0) {
  //  RAISE_ERRNO(kIOError, "sigaction() failed");
  //}

  //if (sigaction(SIGILL, &sigact, NULL) < 0) {
  //  RAISE_ERRNO(kIOError, "sigaction() failed");
  //}

  //if (sigaction(SIGFPE, &sigact, NULL) < 0) {
  //  RAISE_ERRNO(kIOError, "sigaction() failed");
  //}
}

