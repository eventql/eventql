/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <stx/exception.h>
#include <stx/exceptionhandler.h>
#include <stx/inspect.h>
#include <stx/logging.h>
#include <stx/StackTrace.h>

namespace stx {

using stx::Exception;

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
    auto rte = dynamic_cast<const stx::Exception&>(error);
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

  StackTrace strace;
  strace.debugPrint(2);

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
  } catch (const stx::Exception& e) {
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

  struct sigaction sigact;
  memset(&sigact, 0, sizeof(sigact));
  sigact.sa_sigaction = &globalSEGVHandler;
  sigact.sa_flags = SA_SIGINFO;

  if (sigaction(SIGSEGV, &sigact, NULL) < 0) {
    RAISE_ERRNO(kIOError, "sigaction() failed");
  }

  if (sigaction(SIGABRT, &sigact, NULL) < 0) {
    RAISE_ERRNO(kIOError, "sigaction() failed");
  }

  if (sigaction(SIGBUS, &sigact, NULL) < 0) {
    RAISE_ERRNO(kIOError, "sigaction() failed");
  }

  if (sigaction(SIGSYS, &sigact, NULL) < 0) {
    RAISE_ERRNO(kIOError, "sigaction() failed");
  }

  if (sigaction(SIGILL, &sigact, NULL) < 0) {
    RAISE_ERRNO(kIOError, "sigaction() failed");
  }

  if (sigaction(SIGFPE, &sigact, NULL) < 0) {
    RAISE_ERRNO(kIOError, "sigaction() failed");
  }
}

} // namespace stx

