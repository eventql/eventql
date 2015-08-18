/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "Document.h"
#include <stx/inspect.h>

using namespace stx;

namespace zbase {

Document::Document(const DocID& id) : id_(id) {}

const DocID& Document::docID() const {
  return id_;
}

const HashMap<String, String>& Document::fields() const {
  return fields_;
}

void Document::setField(const String& field, const String& value) {
  fields_[field] = value;
}

void Document::debugPrint() const {
  stx::iputs("------ BEGIN DOC -------\n    id=$0", id_.docid);
  for (const auto& f : fields_) {
    String abbr = f.second.substr(0, 80);
    StringUtil::replaceAll(&abbr, "\n", " ");
    stx::iputs("    $0=$1", f.first, abbr);
  }
  stx::iputs("------- END DOC --------", 0);
}

} // namespace zbase
