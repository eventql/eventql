/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_FEATUREINDEXWRITER_H
#define _CM_FEATUREINDEXWRITER_H
#include <mutex>
#include <stdlib.h>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#include "stx/stdtypes.h"
#include "stx/mdb/MDB.h"
#include "stx/stats/stats.h"
#include "stx/util/binarymessagereader.h"
#include "stx/util/binarymessagewriter.h"
#include "IndexChangeRequest.h"
#include <inventory/ItemRef.h>
#include "Document.h"

using namespace stx;

namespace zbase {
class CustomerNamespace;

class DocIndex : public RefCounted {
public:

  DocIndex(
      const String& db_path,
      const String& index_name,
      bool readonly);

  ~DocIndex();

  RefPtr<Document> findDocument(const DocID& docid);
  void listDocuments(Function<bool (const DocID& id)> fn);

  Option<String> getField(const DocID& docid, const String& feature);

  //void updateDocument(const IndexChangeRequest& index_request);

  void updateDocument(
      const DocID& docid,
      const Vector<Pair<String, String>>& fields);

  void commit(bool sync = false);

  void saveCursor(const void* data, size_t size);
  Option<Buffer> getCursor() const;

  RefPtr<mdb::MDB> getDBHanndle() const;

protected:
  uint32_t getOrCreateFieldID(const String& field_name);

  void writeDocument(
      const DocID& docid,
      const HashMap<uint32_t, String>& fields);

  void readDocument(
      const DocID& docid,
      HashMap<uint32_t, String>* fields,
      const Set<uint32_t>& select = Set<uint32_t>{});

  bool readonly_;
  RefPtr<mdb::MDB> db_;
  RefPtr<mdb::MDBTransaction> txn_;
  uint32_t max_field_id_;
  HashMap<String, uint32_t> field_ids_;
};

} // namespace zbase

#endif
