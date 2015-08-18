/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "DocIndex.h"
#include "IndexChangeRequest.h"
#include <stx/inspect.h>

using namespace stx;

namespace zbase {

DocIndex::DocIndex(
    const String& db_path,
    const String& index_name,
    bool readonly) :
    readonly_(readonly),
    db_(nullptr),
    txn_(nullptr),
    max_field_id_(0) {
  mdb::MDBOptions mdb_opts;
  mdb_opts.data_filename = index_name + ".db";
  mdb_opts.lock_filename = index_name + ".db.lck";
  mdb_opts.maxsize = 68719476736lu; // 64 GB

  db_ = mdb::MDB::open(db_path, mdb_opts);
  txn_ = db_->startTransaction(readonly_);

  Buffer key;
  Buffer value;
  auto cursor = txn_->getCursor();
  for (int i = 0; ; ++i) {
    if (i == 0) {
      key.append("__f_");
      if (!cursor->getFirstOrGreater(&key, &value)) {
        break;
      }
    } else {
      if (!cursor->getNext(&key, &value)) {
        break;
      }
    }

    if (!StringUtil::beginsWith(key.toString(), "__f_")) {
      break;
    }

    field_ids_.emplace(
        key.toString().substr(4),
        std::stoul(value.toString()));
  }

  cursor->close();
}

DocIndex::~DocIndex() {
  if (txn_.get()) {
    txn_->abort();
  }
}

void DocIndex::commit(bool sync /* = false */) {
  txn_->commit();
  if (sync) {
    db_->sync();
  }

  txn_ = db_->startTransaction(readonly_);
}

Option<String> DocIndex::getField(
    const DocID& docid,
    const String& field) {
  auto fid = field_ids_.find(field);
  if (fid == field_ids_.end()) {
    RAISEF(kIndexError, "unknown field: $0", field);
  }

  HashMap<uint32_t, String> doc;
  Set<uint32_t> fields = { fid->second };
  readDocument(docid, &doc, fields);

  auto res = doc.find(fid->second);
  if (res == doc.end()) {
    return None<String>();
  } else {
    return Some(res->second);
  }
}

void DocIndex::updateDocument(
    const DocID& docid,
    const Vector<Pair<String, String>>& fields) {
  HashMap<uint32_t, String> doc;
  readDocument(docid, &doc);

  bool dirty = false;
  for (const auto& f : fields) {
    auto fid = getOrCreateFieldID(f.first);
    auto& slot = doc[fid];

    if (slot != f.second) {
      dirty = true;
      slot = f.second;
    }
  }

  if (dirty) {
    writeDocument(docid, doc);
  }
}

uint32_t DocIndex::getOrCreateFieldID(const String& field_name) {
  auto id = field_ids_.find(field_name);
  if (id == field_ids_.end()) {
    auto new_id = ++max_field_id_;
    txn_->insert("__f_" + field_name, StringUtil::toString(new_id));
    field_ids_.emplace(field_name, new_id);
    return new_id;
  } else {
    return id->second;
  }
}

RefPtr<Document> DocIndex::findDocument(const DocID& docid) {
  RefPtr<Document> doc(new Document(docid));
  return doc;
}


void DocIndex::writeDocument(
    const DocID& docid,
    const HashMap<uint32_t, String>& fields) {
  util::BinaryMessageWriter writer;

  auto offset = 0;
  for (const auto& f : fields) {
    writer.appendVarUInt(f.first);
    writer.appendVarUInt(offset);
    offset += f.second.size();
  }

  writer.appendVarUInt(0);
  writer.appendVarUInt(offset);

  for (const auto& f : fields) {
    writer.append(f.second.data(), f.second.size());
  }

  txn_->update(
      docid.docid.data(),
      docid.docid.size(),
      writer.data(),
      writer.size());
}


void DocIndex::readDocument(
    const DocID& docid,
    HashMap<uint32_t, String>* fields,
    const Set<uint32_t>& select /* = Set<uint32_t>{} */) {
  void* val;
  size_t val_size;

  if (!txn_->get(docid.docid.data(), docid.docid.size(), &val, &val_size)) {
    return;
  }

  Vector<Pair<uint32_t, uint32_t>> offsets;

  util::BinaryMessageReader reader(val, val_size);
  for (;;) {
    auto fid = reader.readVarUInt();
    auto off = reader.readVarUInt();
    offsets.emplace_back(fid, off);

    if (fid == 0) {
      break;
    }
  }

  auto begin = reader.position();

  for (int i = 0; i < offsets.size() - 1; ++i) {
    auto fid = offsets[i].first;

    if (select.empty() || select.count(fid) > 0) {
      fields->emplace(
          fid,
          String(
              (char*) val + begin + offsets[i].second,
              offsets[i + 1].second - offsets[i].second));
    }
  }
}

void DocIndex::saveCursor(const void* data, size_t size) {
  String key = "__cursor";
  txn_->update(key.data(), key.size(), data, size);
}

Option<Buffer> DocIndex::getCursor() const {
  return txn_->get("__cursor");
}

RefPtr<mdb::MDB> DocIndex::getDBHanndle() const {
  return db_;
}

//
//  FeaturePack features;
//  for (const auto& group : schema_.groupIDs()) {
//    auto db_key = featureDBKey(docid, group);
//    auto buf = txn_->get(db_key);
//
//#ifndef FNORD_NOTRACE
//  stx::logTrace(
//      "cm.featureindex",
//      "Read from featuredb with key=$0 returned $1 bytes",
//      db_key,
//      buf.isEmpty() ? 0 : buf.get().size());
//#endif
//
//    if (buf.isEmpty()) {
//      continue;
//    }
//
//    FeaturePackReader pack(buf.get().data(), buf.get().size());
//    pack.readFeatures(&features);
//  }
//
//  for (const auto& f : features) {
//    auto fkey = schema_.featureKey(f.first);
//    if (fkey.isEmpty()) {
//      continue;
//    }
//
//    doc->setField(fkey.get(), f.second);
//  }
//
//  return doc;
//}
//
//void DocIndex::updateIndex(const IndexChangeRequest& index_request) {
//  Vector<Pair<FeatureID, String>> features;
//  for (const auto& p : index_request.attrs) {
//    auto fid = schema_.featureID(p.first);
//
//    if (fid.isEmpty()) {
//      continue;
//    }
//
//    features.emplace_back(fid.get(), p.second);
//  }
//
//  auto docid = index_request.item.docID();
//  updateIndex(docid, features);
//}
//
//void DocIndex::updateIndex(
//    const DocID& docid,
//    const Vector<Pair<FeatureID, String>>& features) {
//  Set<uint64_t> groups;
//  for (const auto& p : features) {
//    groups.emplace(p.first.group);
//  }
//
//  for (const auto& group : groups) {
//    FeaturePack group_features;
//    auto db_key = featureDBKey(docid, group);
//
//    auto old_buf = txn_->get(db_key);
//    if (!old_buf.isEmpty()) {
//      FeaturePackReader old_pack(old_buf.get().data(), old_buf.get().size());
//      old_pack.readFeatures(&group_features);
//    }
//
//    for (const auto& p : features) {
//      if (p.first.group != group) {
//        continue;
//      }
//
//      bool found = false;
//      for (auto& gp : group_features) {
//        if (gp.first.feature == p.first.feature) {
//          gp.second = p.second;
//          found = true;
//          break;
//        }
//      }
//
//      if (!found) {
//        group_features.emplace_back(p.first, p.second);
//      }
//    }
//
//    sortFeaturePack(group_features);
//
//    FeaturePackWriter pack;
//    pack.writeFeatures(group_features);
//
//    /* fastpath for noop updates */
//    if (!old_buf.isEmpty() &&
//        pack.size() == old_buf.get().size() &&
//        memcmp(old_buf.get().data(), pack.data(), pack.size()) == 0) {
//      continue;
//    }
//
//    txn_->update(db_key, pack.data(), pack.size());
//  }
//}
//
//void DocIndex::listDocuments(Function<bool (const DocID& id)> fn) {
//  Buffer key;
//  Buffer value;
//  String last_key;
//
//  auto cursor = txn_->getCursor();
//  for (int n = 0; ; ) {
//    bool eof;
//    if (n++ == 0) {
//      eof = !cursor->getFirst(&key, &value);
//    } else {
//      eof = !cursor->getNext(&key, &value);
//    }
//
//    if (eof) {
//      break;
//    }
//
//    auto key_str = key.toString();
//    if (StringUtil::beginsWith(key_str, "__")) {
//      continue;
//    }
//
//    key_str.erase(key_str.find("~", 3), std::string::npos);
//    if (key_str == last_key) {
//      continue;
//    }
//
//    last_key = key_str;
//    if (!fn(DocID { .docid = key_str })) {
//      break;
//    }
//  }
//
//  cursor->close();
//}
//
//void DocIndex::getFields(const DocID& docid, FeaturePack* features) {
//  for (const auto& group : schema_.groupIDs()) {
//    auto db_key = featureDBKey(docid, group);
//    auto buf = txn_->get(db_key);
//
//#ifndef FNORD_NOTRACE
//  stx::logTrace(
//      "cm.featureindex",
//      "Read from featuredb with key=$0 returned $1 bytes",
//      db_key,
//      buf.isEmpty() ? 0 : buf.get().size());
//#endif
//
//    if (buf.isEmpty()) {
//      continue;
//    }
//
//    FeaturePackReader pack(buf.get().data(), buf.get().size());
//    pack.readFeatures(features);
//  }
//}
//

//
//Option<String> DocIndex::getField(
//    const DocID& docid,
//    const FeatureID& featureid) {
//  auto db_key = featureDBKey(docid, featureid.group);
//
//  //auto cached = cache_.get(db_key);
//  //if (!cached.isEmpty()) {
//  //  return cached;
//  //}
//
//  auto buf = txn_->get(db_key);
//
//#ifndef FNORD_NOTRACE
//stx::logTrace(
//    "cm.featureindex",
//    "Read from featuredb with key=$0 returned $1 bytes",
//    db_key,
//    buf.isEmpty() ? 0 : buf.get().size());
//#endif
//
//  if (buf.isEmpty()) {
//    return None<String>();
//  }
//
//  FeaturePack features;
//  FeaturePackReader pack(buf.get().data(), buf.get().size());
//  pack.readFeatures(&features);
//
//  for (const auto& f : features) {
//    if (f.first.feature == featureid.feature) {
//      //cache_.store(db_key, f.second);
//      return Some(f.second);
//    }
//  }
//
//  return None<String>();
//}


} // namespace zbase

