/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <docdb/Document.h>

using namespace stx;

namespace zbase {

Option<DocumentACL> findDocumentACLForUser(
    const Document& doc,
    const String& userid) {
  for (const auto& acl : doc.acls()) {
    if (acl.userid() == userid) {
      return Some(acl);
    }
  }

  return None<DocumentACL>();
}

bool isDocumentReadableForUser(const Document& doc, const String& userid) {
  switch (doc.acl_policy()) {
    case ACLPOLICY_PUBLIC_INTERNET:
    case ACLPOLICY_PUBLIC_ORGANIZATION:
      return true;
    default:
      break;
  }

  auto acls = findDocumentACLForUser(doc, userid);
  return !acls.isEmpty() && acls.get().allow_read();
}

bool isDocumentWritableForUser(const Document& doc, const String& userid) {
  auto acls = findDocumentACLForUser(doc, userid);
  return !acls.isEmpty() && acls.get().allow_write();
}

void setDefaultDocumentACLs(Document* doc, const String& userid) {
  doc->set_acl_policy(ACLPOLICY_PRIVATE);

  auto owner_acl = doc->add_acls();
  owner_acl->set_userid(userid);
  owner_acl->set_is_owner(true);
  owner_acl->set_allow_read(true);
  owner_acl->set_allow_write(true);
  owner_acl->set_allow_share(true);
}

}
