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

enum class DeviceType : uint32_t {
  UNKNOWN = 0,
  DESKTOP = 1,
  PHONE = 2,
  TABLET = 3
};

const uint32_t kMaxDeviceType = 3;

enum class PageType : uint32_t {
  UNKNOWN = 0,
  SEARCH_PAGE = 1,
  CATALOG_PAGE = 2,
  PRODUCT_PAGE = 3
};

const uint32_t kMaxPageType = 3;

std::string cmHostname();

bool isReservedPixelParam(const std::string param);

Option<String> extractAttr(const Vector<String>& attrs, const String& attr);
String extractDeviceTypeString(const Vector<String>& attrs);
DeviceType extractDeviceType(const Vector<String>& attrs);
String extractTestGroup(const Vector<String>& attrs);
Option<uint32_t> extractABTestGroup(const Vector<String>& attrs);
Language extractLanguage(const Vector<String>& attrs);
String extractPageTypeString(const Vector<String>& attrs);
PageType extractPageType(const Vector<String>& attrs);
Option<String> extractQueryString(const Vector<String>& attrs);

String joinBagOfWords(const Set<String>& words);

String deviceTypeToString(DeviceType device_type);
DeviceType deviceTypeFromString(const String& device_type);
PageType pageTypeFromString(const String& page_type);

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
