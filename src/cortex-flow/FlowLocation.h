// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-flow/Api.h>
#include <string>

namespace cortex {
namespace flow {

//! \addtogroup Flow
//@{

struct CORTEX_FLOW_API FilePos  // {{{
    {
  FilePos() : line(1), column(1), offset(0) {}
  FilePos(size_t r, size_t c, size_t o) : line(r), column(c), offset(o) {}

  FilePos& set(size_t r, size_t c, size_t o) {
    line = r;
    column = c;
    offset = o;

    return *this;
  }

  size_t line;
  size_t column;
  size_t offset;
};

inline CORTEX_FLOW_API size_t operator-(const FilePos& a, const FilePos& b) {
  if (b.offset > a.offset)
    return 1 + b.offset - a.offset;
  else
    return 1 + a.offset - b.offset;
}
// }}}
struct CORTEX_FLOW_API FlowLocation  // {{{
    {
  FlowLocation() : filename(), begin(), end() {}
  FlowLocation(const std::string& _fileName)
      : filename(_fileName), begin(), end() {}
  FlowLocation(const std::string& _fileName, FilePos _beg, FilePos _end)
      : filename(_fileName), begin(_beg), end(_end) {}

  std::string filename;
  FilePos begin;
  FilePos end;

  FlowLocation& update(const FilePos& endPos) {
    end = endPos;
    return *this;
  }
  FlowLocation& update(const FlowLocation& endLocation) {
    end = endLocation.end;
    return *this;
  }

  std::string str() const;
  std::string text() const;
};  // }}}

inline FlowLocation operator-(const FlowLocation& end,
                              const FlowLocation& beg) {
  return FlowLocation(beg.filename, beg.begin, end.end);
}

//!@}

}  // namespace flow
}  // namespace cortex
