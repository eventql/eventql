/**
 * This file is part of the "libcstable" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * libcstable is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <cstable/ColumnWriter.h>

using namespace stx;

namespace cstable {

ColumnWriter::ColumnWriter(
    size_t r_max,
    size_t d_max) :
    r_max_(r_max),
    d_max_(d_max) {}

void ColumnWriter::writeString(
    uint64_t rlvl,
    uint64_t dlvl,
    const String& str) {
  writeString(rlvl, dlvl, str.data(), str.size());
}

size_t ColumnWriter::maxRepetitionLevel() const {
  return r_max_;
}

size_t ColumnWriter::maxDefinitionLevel() const {
  return d_max_;
}

void ColumnWriter::writeDateTime(
    uint64_t rlvl,
    uint64_t dlvl,
    UnixTime value) {
  writeUnsignedInt(rlvl, dlvl, value.unixMicros());
}

DefaultColumnWriter::DefaultColumnWriter(
    ColumnConfig config,
    RefPtr<PageManager> page_mgr,
    RefPtr<PageIndex> page_idx) :
    ColumnWriter(config.rlevel_max, config.dlevel_max),
    config_(config) {
  if (config.rlevel_max > 0) {
    PageIndexKey rlevel_idx_key {
      .column_id = config.column_id,
      .entry_type = PageIndexEntryType::RLEVEL
    };

    rlevel_writer_ = mkScoped(
        new UInt64PageWriter(rlevel_idx_key, page_mgr, page_idx));

    page_idx->addPageWriter(rlevel_idx_key, rlevel_writer_.get());
  }

  if (config.dlevel_max > 0) {
    PageIndexKey dlevel_idx_key {
      .column_id = config.column_id,
      .entry_type = PageIndexEntryType::DLEVEL
    };

    dlevel_writer_ = mkScoped(
        new UInt64PageWriter(dlevel_idx_key, page_mgr, page_idx));

    page_idx->addPageWriter(dlevel_idx_key, dlevel_writer_.get());
  }
}


void DefaultColumnWriter::writeNull(uint64_t rep_level, uint64_t def_level) {
  if (rlevel_writer_.get()) {
    rlevel_writer_->appendValue(rep_level);
  }

  if (dlevel_writer_.get()) {
    dlevel_writer_->appendValue(def_level);
  }
}

} // namespace cstable


