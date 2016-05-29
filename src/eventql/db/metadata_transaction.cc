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
#include "eventql/db/metadata_transaction.h"

namespace eventql {

MetadataTransaction::MetadataTransaction(
    const SHA1Hash& transaction_id,
    uint64_t transaction_seq) :
    transaction_id_(transaction_id),
    transaction_seq_(transaction_seq) {}

const SHA1Hash& MetadataTransaction::getTransactionID() const {
  return transaction_id_;
}

uint64_t MetadataTransaction::getSequenceNumber() const {
  return transaction_seq_;
}

bool MetadataTransaction::operator==(const MetadataTransaction& other) const {
  return transaction_id_ == other.transaction_id_;
}

bool MetadataTransaction::operator!=(const MetadataTransaction& other) const {
  return !(transaction_id_ == other.transaction_id_);
}

} // namespace eventql
