/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_STATS_STAT_H
#define _STX_STATS_STAT_H
#include "stx/autoref.h"
#include "stx/stats/statssink.h"

namespace stx {
namespace stats {

class Stat : public RefCounted {
public:
  virtual ~Stat() {};
  virtual void exportAll(const String& path, StatsSink* sink) const = 0;
};

class StatRef {
public:
  virtual ~StatRef() {};
  virtual RefPtr<Stat> getStat() const = 0;
};

}
}

#endif
