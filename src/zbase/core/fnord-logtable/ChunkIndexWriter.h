/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_LOGTABLE_CHUNKINDEXWRITER_H
#define _FNORD_LOGTABLE_CHUNKINDEXWRITER_H
#include <stx/stdtypes.h>

namespace stx {
namespace cstable {

class ChunkIndexWriter {
public:

  ChunkIndexWriter(const String& directory, bool create);

};

} // namespace logtable
} // namespace stx

#endif
