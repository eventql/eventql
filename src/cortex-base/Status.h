// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <cortex-base/Api.h>
#include <system_error>

namespace cortex {

enum class Status {
  Success = 0,
  BufferOverflowError = 1,
  EncodingError,
  ConcurrentModificationError,
  DivideByZeroError,
  FlagError,
  IOError,
  IllegalArgumentError,
  IllegalFormatError,
  IllegalStateError,
  IndexError,
  InvalidOptionError,
  KeyError,
  MallocError,
  NoSuchMethodError,
  NotImplementedError,
  NotYetImplementedError,
  NullPointerError,
  ParseError,
  RangeError,
  ReflectionError,
  ResolveError,
  RPCError,
  RuntimeError,
  TypeError,
  UsageError,
  VersionMismatchError,
  WouldBlockError,
  FutureError,

  ForeignError,
  InvalidArgumentError,
  InternalError,
  InvalidUriPortError,
  CliTypeMismatchError,
  CliUnknownOptionError,
  CliMissingOptionError,
  CliMissingOptionValueError,
  CliFlagNotFoundError,
  SslPrivateKeyCheckError,
  OptionUncheckedAccessToInstance,
  CaughtUnknownExceptionError,
  ConfigurationError,
};

class StatusCategory : public std::error_category {
 public:
  static StatusCategory& get();

  const char* name() const noexcept override;
  std::string message(int ec) const override;
};

CORTEX_API std::string to_string(Status ec);
CORTEX_API std::error_code makeErrorCode(Status ec);
CORTEX_API void raiseIfError(Status status);

enum StatusCompat { // {{{
  kBufferOverflowError = (int) Status::BufferOverflowError,
  kEncodingError = (int) Status::EncodingError,
  kConcurrentModificationError = (int) Status::ConcurrentModificationError,
  kDivideByZeroError = (int) Status::DivideByZeroError,
  kFlagError = (int) Status::FlagError,
  kIOError = (int) Status::IOError,
  kIllegalArgumentError = (int) Status::IllegalArgumentError,
  kIllegalFormatError = (int) Status::IllegalFormatError,
  kIllegalStateError = (int) Status::IllegalStateError,
  kIndexError = (int) Status::IndexError,
  kInvalidOptionError = (int) Status::InvalidOptionError,
  kKeyError = (int) Status::KeyError,
  kMallocError = (int) Status::MallocError,
  kNoSuchMethodError = (int) Status::NoSuchMethodError,
  kNotImplementedError = (int) Status::NotImplementedError,
  kNotYetImplementedError = (int) Status::NotYetImplementedError,
  kNullPointerError = (int) Status::NullPointerError,
  kParseError = (int) Status::ParseError,
  kRangeError = (int) Status::RangeError,
  kReflectionError = (int) Status::ReflectionError,
  kResolveError = (int) Status::ResolveError,
  kRPCError = (int) Status::RPCError,
  kRuntimeError = (int) Status::RuntimeError,
  kTypeError = (int) Status::TypeError,
  kUsageError = (int) Status::UsageError,
  kVersionMismatchError = (int) Status::VersionMismatchError,
  kWouldBlockError = (int) Status::WouldBlockError,
  kFutureError = (int) Status::FutureError
}; // }}}

}  // namespace cortex
