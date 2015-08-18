/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/Report.h"
#include "zbase/shop_stats/ShopStats.h"

namespace cm {

static double mkRate(double num, double div) {
  if (div == 0 || num == 0) {
    return 0;
  }

  return num / div;
}

void ShopStats::merge(const ShopKPIs& src, ShopKPIs* dst) {
  pb_nmerge(*dst, src, product_page_impressions);
  pb_nmerge(*dst, src, catalog_impressions);
  pb_nmerge(*dst, src, catalog_listview_impressions);
  pb_nmerge(*dst, src, catalog_clicks);
  pb_nmerge(*dst, src, catalog_listview_clicks);
  pb_nmerge(*dst, src, search_impressions);
  pb_nmerge(*dst, src, search_listview_impressions);
  pb_nmerge(*dst, src, search_clicks);
  pb_nmerge(*dst, src, search_listview_clicks);
  pb_nmerge(*dst, src, shop_page_impressions);
  pb_nmerge(*dst, src, shop_page_listview_impressions);
  pb_nmerge(*dst, src, shop_page_clicks);
  pb_nmerge(*dst, src, shop_page_listview_clicks);
  pb_nmerge(*dst, src, ad_impressions);
  pb_nmerge(*dst, src, ad_clicks);
  pb_nmerge(*dst, src, unique_purchases);
  pb_nmerge(*dst, src, refunded_purchases);
  pb_nmerge(*dst, src, gmv_eurcent);
  pb_nmerge(*dst, src, refunded_gmv_eurcent);
  pb_nmerge(*dst, src, order_items_count);
  pb_nmerge(*dst, src, refunded_order_items_count);
  pb_nmerge(*dst, src, cart_items_count);
}

Vector<String> ShopStats::columns(const ShopKPIs& src) {
  return Vector<String> {
    "product_page_impressions",
    "product_listview_impressions",
    "product_listview_clicks",
    "product_listview_ctr",
    "product_buy_to_detail_rate",
    "product_cart_to_detail_rate",
    "unique_purchases",
    "refunded_purchases",
    "refund_rate",
    "gmv_eurcent",
    "refunded_gmv_eurcent",
    "gmv_per_transaction",
    "order_items_count",
    "refunded_order_items_count",
    "catalog_impressions",
    "catalog_clicks",
    "catalog_ctr",
    "catalog_listview_impressions",
    "catalog_listview_clicks",
    "catalog_listview_ctr",
    "search_impressions",
    "search_clicks",
    "search_ctr",
    "search_listview_impressions",
    "search_listview_clicks",
    "search_listview_ctr",
    "shop_page_impressions",
    "shop_page_clicks",
    "shop_page_ctr",
    "shop_page_listview_impressions",
    "shop_page_listview_clicks",
    "shop_page.listview_ctr",
    "ad_impressions",
    "ad_clicks",
    "ad_ctr"
  };
}

void ShopStats::toRow(const ShopKPIs& src, Vector<csql::SValue>* dst) {
  dst->emplace_back(csql::SValue::IntegerType(src.product_page_impressions()));

  auto total_listview_impressions =
      src.catalog_listview_impressions() +
      src.search_listview_impressions() +
      src.shop_page_listview_impressions();

  auto total_listview_clicks =
      src.catalog_listview_clicks() +
      src.search_listview_clicks() +
      src.shop_page_listview_clicks();

  dst->emplace_back(csql::SValue::IntegerType(total_listview_impressions));
  dst->emplace_back(csql::SValue::IntegerType(total_listview_clicks));
  dst->emplace_back(csql::SValue::FloatType(
      mkRate(total_listview_clicks, total_listview_impressions)));

  dst->emplace_back(csql::SValue::FloatType(
      mkRate(src.order_items_count(), src.product_page_impressions())));

  dst->emplace_back(csql::SValue::FloatType(
      mkRate(src.cart_items_count(), src.product_page_impressions())));

  dst->emplace_back(csql::SValue::IntegerType(src.unique_purchases()));
  dst->emplace_back(csql::SValue::IntegerType(src.refunded_purchases()));
  dst->emplace_back(csql::SValue::FloatType(
      mkRate(src.refunded_purchases(), src.unique_purchases())));

  dst->emplace_back(csql::SValue::IntegerType(src.gmv_eurcent()));
  dst->emplace_back(csql::SValue::IntegerType(src.refunded_gmv_eurcent()));
  dst->emplace_back(csql::SValue::FloatType(
      mkRate(src.gmv_eurcent(), src.unique_purchases())));

  dst->emplace_back(csql::SValue::IntegerType(src.order_items_count()));
  dst->emplace_back(csql::SValue::IntegerType(src.refunded_order_items_count()));

  dst->emplace_back(csql::SValue::IntegerType(src.catalog_impressions()));
  dst->emplace_back(csql::SValue::IntegerType(src.catalog_clicks()));
  dst->emplace_back(csql::SValue::FloatType(
      mkRate(src.catalog_clicks(), src.catalog_impressions())));
  dst->emplace_back(csql::SValue::IntegerType(src.catalog_listview_impressions()));
  dst->emplace_back(csql::SValue::IntegerType(src.catalog_listview_clicks()));
  dst->emplace_back(csql::SValue::FloatType(
      mkRate(src.catalog_listview_clicks(), src.catalog_listview_impressions())));

  dst->emplace_back(csql::SValue::IntegerType(src.search_impressions()));
  dst->emplace_back(csql::SValue::IntegerType(src.search_clicks()));
  dst->emplace_back(csql::SValue::FloatType(
      mkRate(src.search_clicks(), src.search_impressions())));
  dst->emplace_back(csql::SValue::IntegerType(src.search_listview_impressions()));
  dst->emplace_back(csql::SValue::IntegerType(src.search_listview_clicks()));
  dst->emplace_back(csql::SValue::FloatType(
      mkRate(src.search_listview_clicks(), src.search_listview_impressions())));

  dst->emplace_back(csql::SValue::IntegerType(src.shop_page_impressions()));
  dst->emplace_back(csql::SValue::IntegerType(src.shop_page_clicks()));
  dst->emplace_back(csql::SValue::FloatType(
      mkRate(src.shop_page_clicks(), src.shop_page_impressions())));
  dst->emplace_back(csql::SValue::IntegerType(src.shop_page_listview_impressions()));
  dst->emplace_back(csql::SValue::IntegerType(src.shop_page_listview_clicks()));
  dst->emplace_back(csql::SValue::FloatType(
      mkRate(src.shop_page_listview_clicks(), src.shop_page_listview_impressions())));

  dst->emplace_back(csql::SValue::IntegerType(src.ad_impressions()));
  dst->emplace_back(csql::SValue::IntegerType(src.ad_clicks()));
  dst->emplace_back(csql::SValue::FloatType(
      mkRate(src.ad_clicks(), src.ad_impressions())));
}

void ShopStats::merge(const ShopProductKPIs& src, ShopProductKPIs* dst) {
  pb_nmerge(*dst, src, product_page_impressions);
  pb_nmerge(*dst, src, catalog_listview_impressions);
  pb_nmerge(*dst, src, catalog_listview_clicks);
  pb_nmerge(*dst, src, search_listview_impressions);
  pb_nmerge(*dst, src, search_listview_clicks);
  pb_nmerge(*dst, src, shop_page_listview_impressions);
  pb_nmerge(*dst, src, shop_page_listview_clicks);
  pb_nmerge(*dst, src, ad_impressions);
  pb_nmerge(*dst, src, ad_clicks);
  pb_nmerge(*dst, src, unique_purchases);
  pb_nmerge(*dst, src, refunded_purchases);
  pb_nmerge(*dst, src, gmv_eurcent);
  pb_nmerge(*dst, src, refunded_gmv_eurcent);
  pb_nmerge(*dst, src, cart_items_count);
}

Vector<String> ShopStats::columns(const ShopProductKPIs& src) {
  return Vector<String> {
    "product_page_impressions",
    "product_listview_impressions",
    "product_listview_clicks",
    "product_listview_ctr",
    "product_buy_to_detail_rate",
    "product_cart_to_detail_rate",
    "unique_purchases",
    "refunded_purchases",
    "refund_rate",
    "gmv_eurcent",
    "refunded_gmv_eurcent",
    "catalog_listview_impressions",
    "catalog_listview_clicks",
    "catalog_listview_ctr",
    "search_listview_impressions",
    "search_listview_clicks",
    "search_listview_ctr",
    "shop_page_listview_impressions",
    "shop_page_listview_clicks",
    "shop_page_listview_ctr",
    "ad_impressions",
    "ad_clicks",
    "ad_ctr"
  };
}

void ShopStats::toRow(const ShopProductKPIs& src, Vector<csql::SValue>* dst) {
  dst->emplace_back(csql::SValue::IntegerType(src.product_page_impressions()));

  auto total_listview_impressions =
      src.catalog_listview_impressions() +
      src.search_listview_impressions() +
      src.shop_page_listview_impressions();

  auto total_listview_clicks =
      src.catalog_listview_clicks() +
      src.search_listview_clicks() +
      src.shop_page_listview_clicks();

  dst->emplace_back(csql::SValue::IntegerType(total_listview_impressions));
  dst->emplace_back(csql::SValue::IntegerType(total_listview_clicks));
  dst->emplace_back(csql::SValue::FloatType(
      mkRate(total_listview_clicks, total_listview_impressions)));

  dst->emplace_back(csql::SValue::FloatType(
      mkRate(src.unique_purchases(), src.product_page_impressions())));

  dst->emplace_back(csql::SValue::FloatType(
      mkRate(src.cart_items_count(), src.product_page_impressions())));

  dst->emplace_back(csql::SValue::IntegerType(src.unique_purchases()));
  dst->emplace_back(csql::SValue::IntegerType(src.refunded_purchases()));
  dst->emplace_back(csql::SValue::FloatType(
      mkRate(src.refunded_purchases(), src.unique_purchases())));

  dst->emplace_back(csql::SValue::IntegerType(src.gmv_eurcent()));
  dst->emplace_back(csql::SValue::IntegerType(src.refunded_gmv_eurcent()));

  dst->emplace_back(csql::SValue::IntegerType(src.catalog_listview_impressions()));
  dst->emplace_back(csql::SValue::IntegerType(src.catalog_listview_clicks()));
  dst->emplace_back(csql::SValue::FloatType(
      mkRate(src.catalog_listview_clicks(), src.catalog_listview_impressions())));

  dst->emplace_back(csql::SValue::IntegerType(src.search_listview_impressions()));
  dst->emplace_back(csql::SValue::IntegerType(src.search_listview_clicks()));
  dst->emplace_back(csql::SValue::FloatType(
      mkRate(src.search_listview_clicks(), src.search_listview_impressions())));

  dst->emplace_back(csql::SValue::IntegerType(src.shop_page_listview_impressions()));
  dst->emplace_back(csql::SValue::IntegerType(src.shop_page_listview_clicks()));
  dst->emplace_back(csql::SValue::FloatType(
      mkRate(src.shop_page_listview_clicks(), src.shop_page_listview_impressions())));

  dst->emplace_back(csql::SValue::IntegerType(src.ad_impressions()));
  dst->emplace_back(csql::SValue::IntegerType(src.ad_clicks()));
  dst->emplace_back(csql::SValue::FloatType(
      mkRate(src.ad_clicks(), src.ad_impressions())));
}

} // namespace cm
