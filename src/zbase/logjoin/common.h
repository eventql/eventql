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
#include <stx/option.h>
#include <stx/stdtypes.h>
#include <stx/Language.h>
#include <stx/Currency.h>
#include <stx/util/CumulativeHistogram.h>
#include <stx/json/json.h>

using namespace stx;

/**
 * mandatory global params:
 *   v           -- pixel ver.  -- value: 1
 *   c           -- clickid     -- format "<uid>~<eventid>", e.g. "f97650cb~b28c61d5c"
 *   e           -- eventtype   -- format "{q,v,c,u}" (query, visit, cart, user)
 *
 * optional global params:
 *   dw_ab      -- dawanda a/b grp  -- format "0-100"
 *   l          -- page language    -- format "<lang>"
 *   u_x        -- screen width     -- format "<num>"
 *   u_y        -- screen height    -- format "<num>"
 *   x          -- experiment       -- format "<exp1>;<exp2>"
 *
 * params for eventtype=q (query):
 *   is         -- item ids         -- format "<setid>~<itemid>[~p<pos>][~s],..."
 *   pg         -- page number      -- format "<num>"
 *   qstr~<lc>  -- query string     -- format "<string>"
 *   q_cat1     -- query cat1       -- format "<catid>"
 *   q_cat2     -- query cat2       -- format "<catid>"
 *   q_cat3     -- query cat3       -- format "<catid>"
 *   slrid      -- seller id        -- format "<id>"
 *   qx         -- experiment       -- format "<exp1>;<exp2>"
 *   qt         -- query type       -- e.g. "fts/catalog/reco"
 *
 * params for eventtype=v (visit):
 *   i          -- itemid           -- format "<setid>~<itemid>"
 *
 * params for eventtype=c (cart):
 *   s          -- step              -- format "<num>", step 1 is final/confirm step!
 *   is         -- items             -- format "<setid>~<itemid>~<qty>~<cents>~<currency>,..."
 *
 * params for eventtype=u (user):
 *   ml         -- email            -- format "<email>"
 *   adm        -- is_admin         -- format "t/f"
 *   lgn        -- is_logged_in     -- format "t/f"
 *   slr        -- is_seller        -- format "t/f"
 *   lng        -- language         -- format "<lang>"
 *   dwnid      -- dawanda id       -- format "<id>"
 *   fnm        -- first name       -- format "<name>"
 *   r_url      -- referrer url
 *   r_nm       -- referrer name
 *   r_cpn      -- referrer campaign
 *   cs         -- customer session id
 *
 */

namespace zbase {

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
  PRODUCT_PAGE = 3,
  SHOP_PAGE = 4
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
String pageTypeToString(PageType device_type);

CurrencyConverter::ConversionTable currencyConversionTable();
CurrencyConverter* cconv();

bool isIndexAttributeWhitelisted(const String& attr);

}
#endif
