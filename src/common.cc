/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <algorithm>
#include <unistd.h>
#include <fnord-base/UTF8.h>
#include "common.h"

namespace cm {

/**
 * mandatory params:
 *  v    -- pixel ver.  -- value: 1
 *  c    -- clickid     -- format "<uid>~<eventid>", e.g. "f97650cb~b28c61d5c"
 *  e    -- eventtype   -- format "{q,v}" (query, visit)
 *
 * params for eventtype=q (query):
 *  is   -- item ids    -- format "<setid>~<itemid>~<pos>,..."
 *
 * params for eventtype=v (visit):
 *  i    -- itemid      -- format "<setid>~<itemid>"
 *
 */
bool isReservedPixelParam(const std::string p) {
  return p == "c" || p == "e" || p == "i" || p == "is" || p == "v";
}

std::string cmHostname() {
  char hostname[128];
  gethostname(hostname, sizeof(hostname));
  hostname[127] = 0;
  return std::string(hostname);
}

Option<String> extractAttr(const Vector<String>& attrs, const String& attr) {
  auto prefix = attr + ":";

  for (const auto& a : attrs) {
    if (StringUtil::beginsWith(a, prefix)) {
      return Some(a.substr(prefix.length()));
    }
  }

  return None<String>();
}

String extractDeviceTypeString(const Vector<String>& attrs) {
  return deviceTypeToString(extractDeviceType(attrs));
}

DeviceType extractDeviceType(const Vector<String>& attrs) {
  auto x_str = extractAttr(attrs, "u_x");
  auto y_str = extractAttr(attrs, "u_y");

  if (x_str.isEmpty() || y_str.isEmpty()) {
    return DeviceType::UNKNOWN;
  }

  auto x = std::stod(x_str.get());
  auto y = std::stod(x_str.get());

  if (x < 10 || y < 10) {
    return DeviceType::UNKNOWN;
  }

  if (x < 1000) {
    return DeviceType::PHONE;
  }

  if (x < 1400) {
    return DeviceType::TABLET;
  }

  return DeviceType::DESKTOP;
}

String deviceTypeToString(DeviceType device_type) {
  switch (device_type) {
    case DeviceType::UNKNOWN: return "unknown";
    case DeviceType::PHONE: return "phone";
    case DeviceType::TABLET: return "tablet";
    case DeviceType::DESKTOP: return "desktop";
  }
}

DeviceType deviceTypeFromString(const String& device_type) {
  if (device_type == "phone") {
    return DeviceType::PHONE;
  }

  if (device_type == "tablet") {
    return DeviceType::TABLET;
  }

  if (device_type == "desktop") {
    return DeviceType::DESKTOP;
  }

  return DeviceType::UNKNOWN;
}

String extractTestGroup(const Vector<String>& attrs) {
  auto test_group = extractAttr(attrs, "dw_ab");
  return test_group.isEmpty() ? "unknown" : test_group.get();
}

Option<uint32_t> extractABTestGroup(const Vector<String>& attrs) {
  auto test_group = extractAttr(attrs, "dw_ab");

  if (test_group.isEmpty()) {
    return None<uint32_t>();
  } else {
    return Some<uint32_t>(std::stoul(test_group.get()));
  }
}

Language extractLanguage(const Vector<String>& attrs) {
  auto l = extractAttr(attrs, "l");

  if (!l.isEmpty()) {
    return languageFromString(l.get());
  }

  // FIXPAUL hack!!!
  if (!extractAttr(attrs, "qstr~de").isEmpty()) {
    return Language::DE;
  }

  if (!extractAttr(attrs, "qstr~pl").isEmpty()) {
    return Language::PL;
  }

  return Language::UNKNOWN;
}

String extractPageTypeString(const Vector<String>& attrs) {
  for (const auto& a : attrs) {
    if (StringUtil::beginsWith(a, "qstr~")) {
      return "search";
    }

    if (StringUtil::beginsWith(a, "q_cat")) {
      return "catalog";
    }
  }

  return "unknown";
}

PageType extractPageType(const Vector<String>& attrs) {
  return pageTypeFromString(extractPageTypeString(attrs));
}

PageType pageTypeFromString(const String& page_type) {
  if (page_type == "search") {
    return PageType::SEARCH_PAGE;
  }

  if (page_type == "catalog") {
    return PageType::CATALOG_PAGE;
  }

  return PageType::UNKNOWN;
}

String pageTypeToString(PageType page_type) {
  switch (page_type) {
    case PageType::UNKNOWN: return "unknown";
    case PageType::SEARCH_PAGE: return "search_page";
    case PageType::CATALOG_PAGE: return "catalog_page";
    case PageType::PRODUCT_PAGE: return "product_page";
  }
}

String joinBagOfWords(const Set<String>& words) {
  Vector<String> v;

  for (const auto& w : words) {
    if (w.length() == 0) {
      continue;
    }

    v.emplace_back(w);
  }

  std::sort(v.begin(), v.end());

  return StringUtil::join(v, " ");
}

bool isQueryEligible(
    ItemEligibility eligibility,
    const cm::JoinedQuery& query) {
  switch (eligibility) {

    case ItemEligibility::DAWANDA_ALL_NOBOTS: {
      auto pgs = extractAttr(query.attrs, "pg");
      if (pgs.isEmpty()) {
        return true;
      } else {
        auto pg = std::stoul(pgs.get());
        return pg <= 3;
      }
    }

    case ItemEligibility::ALL:
      return true;

  }
}

bool isItemEligible(
    ItemEligibility eligibility,
    const cm::JoinedQuery& query,
    const cm::JoinedQueryItem& item) {
  switch (eligibility) {

    case ItemEligibility::DAWANDA_ALL_NOBOTS:
      return item.position <= 40 && item.position > 0;

    case ItemEligibility::ALL:
      return true;

  }
}

Option<String> extractQueryString(const Vector<String>& attrs) {
  for (const auto& a : attrs) {
    if (StringUtil::beginsWith(a, "qstr~")) {
      auto o = StringUtil::find(a, ':');
      if (o != String::npos) {
        try {
          return Some(URI::urlDecode(a.substr(o + 1)));
        } catch (...) {
          return None<String>();
        }
      }
    }
  }

  return None<String>();
}

}
