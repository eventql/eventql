/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/io/inputstream.h>
#include <stx/VFSFile.h>

namespace stx {

class VFSFileInputStream : public RewindableInputStream {
public:

  /**
   * Create a new InputStream from the provided string
   *
   * @param string the input string
   */
  VFSFileInputStream(RefPtr<VFSFile> data);

  /**
   * Read the next byte from the file. Returns true if the next byte was read
   * and false if the end of the stream was reached.
   *
   * @param target the target char pointer
   */
  bool readNextByte(char* target) override;

  /**
   * Skip the next N bytes in the stream. Returns the number of bytes skipped.
   *
   * @param n_bytes the number of bytes to skip
   */
  size_t skipNextBytes(size_t n_bytes) override;

  /**
   * Check if the end of this input stream was reached. Returns true if the
   * end was reached, false otherwise
   */
  bool eof() override;

  /**
   * Rewind the input stream
   */
  void rewind() override;

  /**
   * Seek to the provided offset in number of bytes from the beginning of the
   * pointed to memory region. Sets the position to EOF if the provided offset
   * is larger than size of the underlying memory regio
   */
  void seekTo(size_t offset) override;

protected:
  RefPtr<VFSFile> data_;
  size_t cur_;
};

}
