/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/protobuf/msg.h"
#include "stx/wallclock.h"
#include "eventql/docdb/DocumentDB.h"

using namespace stx;

namespace zbase {

DocumentDB::DocumentDB(const String& path) {
  mdb::MDBOptions mdb_opts;
  mdb_opts.data_filename = "docdb.db",
  mdb_opts.lock_filename = "docdb.db.lck";
  mdb_opts.duplicate_keys = false;

  db_ = mdb::MDB::open(path, mdb_opts);
}

void DocumentDB::listDocuments(
    const String& db_namespace,
    Function<bool (const Document& doc)> fn) const {
  auto namespace_prefix = db_namespace + "~d~";

  Buffer key;
  Buffer value;

  auto txn = db_->startTransaction(true);
  txn->autoAbort();

  auto cursor = txn->getCursor();
  key.append(namespace_prefix);
  if (!cursor->getFirstOrGreater(&key, &value)) {
    return;
  }

  do {
    if (!StringUtil::beginsWith(key.toString(), namespace_prefix)) {
      break;
    }

    auto doc = msg::decode<Document>(value);
    if (!fn(doc)) {
      break;
    }
  } while (cursor->getNext(&key, &value));

  cursor->close();
}

void DocumentDB::createDocument(
    const String& db_namespace,
    const Document& document) {
  auto db_key = StringUtil::format("$0~d~$1", db_namespace, document.uuid());
  auto buf = msg::encode(document);

  auto txn = db_->startTransaction(false);
  txn->autoAbort();
  txn->insert(db_key.data(), db_key.size(), buf->data(), buf->size());
  txn->commit();
}

bool DocumentDB::fetchDocument(
    const String& db_namespace,
    const String& userid,
    const SHA1Hash& uuid,
    Document* document) {
  auto db_key = StringUtil::format("$0~d~$1", db_namespace, uuid.toString());

  void* data;
  size_t data_size;
  auto txn = db_->startTransaction(true);
  txn->autoAbort();

  if (txn->get(db_key.data(), db_key.size(), &data, &data_size)) {
    auto doc = msg::decode<Document>(data, data_size);

    if (isDocumentReadableForUser(doc, userid)) {
      *document = doc;
    } else {
      RAISE(kAccessDeniedError, "access denied");
    }

    return true;
  } else {
    return false;
  }
}

void DocumentDB::updateDocument(
      const String& db_namespace,
      const SHA1Hash& uuid,
      Function<void (Document* doc)> fn) {
  auto db_key = StringUtil::format("$0~d~$1", db_namespace, uuid.toString());

  auto txn = db_->startTransaction(false);
  txn->autoAbort();

  void* data;
  size_t data_size;
  if (!txn->get(db_key.data(), db_key.size(), &data, &data_size)) {
    RAISE(kNotFoundError, "document not found");
  }

  auto doc = msg::decode<Document>(data, data_size);
  fn(&doc);
  doc.set_mtime(WallClock::unixMicros());

  auto doc_buf = msg::encode<Document>(doc);
  txn->update(db_key.data(), db_key.size(), doc_buf->data(), doc_buf->size());
  txn->commit();
}

void DocumentDB::updateDocumentContent(
    const String& db_namespace,
    const String& userid,
    const SHA1Hash& uuid,
    const String& content) {
  updateDocument(
    db_namespace,
    uuid,
    [&userid, &content] (Document* doc) {
    if (!isDocumentWritableForUser(*doc, userid)) {
      RAISE(kAccessDeniedError, "access denied");
    }

    doc->set_content(content);
  });
}

void DocumentDB::updateDocumentName(
    const String& db_namespace,
    const String& userid,
    const SHA1Hash& uuid,
    const String& name) {
  updateDocument(
    db_namespace,
    uuid,
    [&userid, &name] (Document* doc) {
    if (!isDocumentWritableForUser(*doc, userid)) {
      RAISE(kAccessDeniedError, "access denied");
    }

    doc->set_name(name);
  });
}

void DocumentDB::updateDocumentCategory(
    const String& db_namespace,
    const String& userid,
    const SHA1Hash& uuid,
    const String& category) {
  updateDocument(
    db_namespace,
    uuid,
    [&userid, &category] (Document* doc) {
    if (!isDocumentWritableForUser(*doc, userid)) {
      RAISE(kAccessDeniedError, "access denied");
    }

    doc->set_category(category);
  });
}

void DocumentDB::updateDocumentACLPolicy(
    const String& db_namespace,
    const String& userid,
    const SHA1Hash& uuid,
    DocumentACLPolicy policy) {
  updateDocument(
      db_namespace,
      uuid,
      [&userid, &policy] (Document* doc) {
    if (!isDocumentWritableForUser(*doc, userid)) {
      RAISE(kAccessDeniedError, "access denied");
    }

    doc->set_acl_policy(policy);
  });
}

void DocumentDB::updateDocumentPublishingStatus(
    const String& db_namespace,
    const String& userid,
    const SHA1Hash& uuid,
    DocumentPublishingStatus pstatus) {
  updateDocument(
      db_namespace,
      uuid,
      [&userid, &pstatus] (Document* doc) {
    if (!isDocumentWritableForUser(*doc, userid)) {
      RAISE(kAccessDeniedError, "access denied");
    }

    doc->set_publishing_status(pstatus);
  });
}

void DocumentDB::updateDocumentDeletedStatus(
    const String& db_namespace,
    const String& userid,
    const SHA1Hash& uuid,
    bool deleted) {
  updateDocument(
      db_namespace,
      uuid,
      [&userid, &deleted] (Document* doc) {
    if (!isDocumentWritableForUser(*doc, userid)) {
      RAISE(kAccessDeniedError, "access denied");
    }

    doc->set_deleted(deleted);
  });
}

//void createDocument(
//    const String& db_namespace,
//    const Document& document);
//
//void updateDocument(
//    const String& db_namespace,
//    const Document& document);
//
//bool findDocument(
//    const String& db_namespace,
//    const SHA1Hash& uuid,
//    Document* document);


} // namespace zbase

