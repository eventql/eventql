/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "stx/stdtypes.h"
#include "stx/SHA1.h"
#include "stx/mdb/MDB.h"
#include "docdb/Document.h"

using namespace stx;

namespace cm {

class DocumentDB : public RefCounted {
public:

  DocumentDB(const String& path);

  void listDocuments(
      const String& db_namespace,
      const String& userid,
      Function<bool (const Document& doc)> fn) const;

  void createDocument(
      const String& db_namespace,
      const Document& document);

  bool fetchDocument(
      const String& db_namespace,
      const String& userid,
      const SHA1Hash& uuid,
      Document* document);

  void updateDocumentContent(
      const String& db_namespace,
      const String& userid,
      const SHA1Hash& uuid,
      const String& content);

  void updateDocumentName(
      const String& db_namespace,
      const String& userid,
      const SHA1Hash& uuid,
      const String& name);

  void updateDocumentACLPolicy(
      const String& db_namespace,
      const String& userid,
      const SHA1Hash& uuid,
      DocumentACLPolicy policy);

protected:

  void updateDocument(
      const String& db_namespace,
      const SHA1Hash& uuid,
      Function<void (Document* doc)> fn);

  RefPtr<mdb::MDB> db_;

  //String docPath(DocID docid) const;

  //void loadDocument(RefPtr<Document> doc);
  //void commitDocument(RefPtr<Document> doc);

  //std::mutex update_lock_;
  //String path_;
};

} // namespace cm

