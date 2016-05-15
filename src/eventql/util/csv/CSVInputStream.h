/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#pragma once
#include "eventql/util/stdtypes.h"
#include "eventql/util/io/inputstream.h"

namespace util {

class CSVInputStream {
public:


  /**
   * Open a new csv file input stream from the provided file path. Throws an
   * exception if the file cannot be opened.
   *
   * @param file_path the path to the csv file
   */
  static std::unique_ptr<CSVInputStream> openFile(
      const std::string& file_path,
      char column_separator = ';',
      char row_separator = '\n',
      char quote_char = '"');

  /**
   * Read the next row from the csv file. Returns true if a row was read and
   * false on EOF. May raise an exception.
   */
  virtual bool readNextRow(std::vector<std::string>* target) = 0;

  /**
   * Skip the next row from the csv file. Returns true if a row was skipped and
   * false on EOF. May raise an exception.
   */
  virtual bool skipNextRow() = 0;

  /**
   * Rewind the input stream
   */
  virtual void rewind() = 0;

  /**
   * Return the input stream
   */
  virtual const RewindableInputStream& getInputStream() const = 0;

};

class DefaultCSVInputStream : public CSVInputStream {
public:

  /**
   * Create a new CSVInputStream from the provided InputStream.
   */
  explicit DefaultCSVInputStream(
      std::unique_ptr<RewindableInputStream>&& input_stream,
      char column_separator = ';',
      char row_separator = '\n',
      char quote_char = '"');

  /**
   * Read the next row from the csv file. Returns true if a row was read and
   * false on EOF. May raise an exception.
   */
  bool readNextRow(std::vector<std::string>* target) override;

  /**
   * Skip the next row from the csv file. Returns true if a row was skipped and
   * false on EOF. May raise an exception.
   */
  bool skipNextRow() override;

  /**
   * Rewind the input stream
   */
  void rewind() override;

  /**
   * Return the input stream
   */
  const RewindableInputStream& getInputStream() const override;

protected:

  /**
   * Read the next column from the csv file
   */
  std::string readNextColumn();

  std::unique_ptr<RewindableInputStream> input_;
  const char column_separator_;
  const char row_separator_;
  const char quote_char_;
};

}
