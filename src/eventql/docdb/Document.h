/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/option.h>
#include <eventql/docdb/Document.pb.h>

using namespace stx;

namespace zbase {

Option<DocumentACL> findDocumentACLForUser(
    const Document& doc,
    const String& userid);

String getDocumentOwner(const Document& doc);

bool isDocumentReadableForUser(const Document& doc, const String& userid);

bool isDocumentWritableForUser(const Document& doc, const String& userid);

bool isDocumentShareableForUser(const Document& doc, const String& userid);

bool isDocumentOwnedByUser(const Document& doc, const String& userid);

bool isDocumentAuthoredByUser(const Document& doc, const String& userid);

void setDefaultDocumentACLs(Document* doc, const String& userid);

}
