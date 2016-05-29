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
#include "eventql/db/metadata_operation.h"

namespace eventql {

MetadataOperation::MetadataOperation() {}

MetadataOperation::MetadataOperation(
    const String& db_namespace,
    const String& table_id,
    MetadataOperationType type,
    const SHA1Hash& input_txid,
    const SHA1Hash& output_txid,
    const Buffer& opdata) {
  data_.set_db_namespace(db_namespace);
  data_.set_table_id(table_id);
  data_.set_input_txid(input_txid.data(), input_txid.size());
  data_.set_output_txid(output_txid.data(), input_txid.size());
  data_.set_optype(type);
  data_.set_opdata(opdata.data(), opdata.size());
}

SHA1Hash MetadataOperation::getInputTransactionID() const {
  return SHA1Hash(data_.input_txid().data(), data_.input_txid().size());
}

SHA1Hash MetadataOperation::getOutputTransactionID() const {
  return SHA1Hash(data_.output_txid().data(), data_.output_txid().size());
}

Status MetadataOperation::decode(InputStream* is) {
  return Status(eRuntimeError, "not yet implemented");
}

Status MetadataOperation::encode(OutputStream* os) const {
  return Status(eRuntimeError, "not yet implemented");
}

} // namespace eventql

