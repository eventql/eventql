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
#include "eventql/db/metadata_service.h"

namespace eventql {

MetadataService::MetadataService(
    ConfigDirectory* cdir,
    MetadataStore* metadata_store) :
    metadata_store_(metadata_store),
    cdir_(cdir) {}

Status MetadataService::getMetadataFile(
    const String& ns,
    const String& table_name,
    RefPtr<MetadataFile>* file) const {
  auto table_cfg = cdir_->getTableConfig(ns, table_name);
  return metadata_store_->getMetadataFile(
      ns,
      table_name,
      SHA1Hash(
          table_cfg.metadata_txnid().data(),
          table_cfg.metadata_txnid().size()),
      file);
}

Status MetadataService::createMetadataFile(
    const String& ns,
    const String& table_name,
    const SHA1Hash& txid) {
  MetadataFile metadata_file(txid, 1, {});
  return metadata_store_->storeMetadataFile(
      ns,
      table_name,
      txid,
      metadata_file);
}

Status MetadataService::performMetadataOperation(
    const String& ns,
    const String& table_name,
    MetadataOperation op) {
  return Status(eRuntimeError, "not yet implemented");
}

Status MetadataService::discoverPartition(
    const PartitionDiscoveryRequest& request,
    PartitionDiscoveryResponse* response) {
  RefPtr<MetadataFile> file;
  {
    auto rc = getMetadataFile(
        request.db_namespace(),
        request.table_id(),
        &file);
    if (!rc.isSuccess()) {
      return rc;
    }
  }


  if (file->getSequenceNumber() < request.min_txnseq()) {
    return Status(eRuntimeError, "metadata file is too old");
  }

  return Status(eRuntimeError, "not yet implemented");
}

} // namespace eventql

