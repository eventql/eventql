/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_COMMON_H
#define _CM_COMMON_H
#include <string>
#include <fnord-base/option.h>
#include <fnord-base/stdtypes.h>
#include <fnord-base/Language.h>
#include "JoinedQuery.h"

using namespace fnord;

namespace cm {

std::string cmHostname();

bool isReservedPixelParam(const std::string param);

Option<String> extractAttr(const Vector<String>& attrs, const String& attr);
String extractDeviceType(const Vector<String>& attrs);
String extractTestGroup(const Vector<String>& attrs);
Language extractLanguage(const Vector<String>& attrs);
String extractPageType(const Vector<String>& attrs);

String joinBagOfWords(const Set<String>& words);

enum class FeaturePrep {
  NONE,
  BAGOFWORDS_DE
};

enum class ItemEligibility {
  ALL = 0,
  DAWANDA_ALL_NOBOTS = 1
};

bool isQueryEligible(ItemEligibility eligibility, const cm::JoinedQuery& query);

bool isItemEligible(
    ItemEligibility eligibility,
    const cm::JoinedQuery& query,
    const cm::JoinedQueryItem& item);

}
#endif
