/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/option.h>
#include <docdb/Document.pb.h>

using namespace stx;

namespace cm {

Option<DocumentACL> findDocumentACLForUser(
    const Document& doc,
    const String& userid);

bool isDocumentReadableForUser(const Document& doc, const String& userid);

bool isDocumentWritableForUser(const Document& doc, const String& userid);

bool isDocumentShareableForUser(const Document& doc, const String& userid);

void setDefaultDocumentACLs(Document* doc, const String& userid);

}
