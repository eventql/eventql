/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "Document.h"

using namespace fnord;

namespace cm {

Document::Document(const DocID& id) : id_(id) {}

const DocID& Document::docID() const {
  return id_;
}

const HashMap<String, String>& Document::fields() const {
  return fields_;
}

void Document::update(const IndexChangeRequest& index_req) {
  for (const auto& a : index_req.attrs) {
    fields_[a.first] = a.second;
  }
}

void Document::setField(const String& field, const String& value) {
  fields_[field] = value;
}

void Document::debugPrint() const {
  fnord::iputs("------ BEGIN DOC -------\n    id=$0", id_.docid);
  for (const auto& f : fields_) {
    String abbr = f.second.substr(0, 80);
    StringUtil::replaceAll(&abbr, "\n", " ");
    fnord::iputs("    $0=$1", f.first, abbr);
  }
  fnord::iputs("------- END DOC --------", 0);
}

} // namespace cm
