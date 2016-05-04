/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "common.h"
#include "FeatureSelector.h"

using namespace stx;

namespace zbase {

FeatureSelector::FeatureSelector(
      FeatureIndex* feature_index,
      stx::fts::Analyzer* analyzer) :
      feature_index_(feature_index),
      analyzer_(analyzer) {}

void FeatureSelector::featuresFor(
    const JoinedQuery& query,
    const JoinedQueryItem& item,
    Set<String>* features) {
  auto docid = item.item.docID();
  features->emplace(StringUtil::format("docid:$0", docid.docid));

  /* item attributes */
  auto shop_id = feature_index_->getFeature(docid, "shop_id");
  if (!shop_id.isEmpty()) {
    features->emplace(StringUtil::format("shop_id:$0", shop_id.get()));
  }

  /* query attributes */
  //auto cat1_id = extractAttr(query.attrs, "q_cat1");
  //if (!cat1_id.isEmpty()) {
  //  features->emplace(StringUtil::format("q_cat1:$0", cat1_id.get()));
  //}

  auto cat2_id = extractAttr(query.attrs, "q_cat2");
  if (!cat2_id.isEmpty()) {
    features->emplace(StringUtil::format("q_cat2:$0", cat2_id.get()));
  }

  //auto cat3_id = extractAttr(query.attrs, "q_cat3");
  //if (!cat3_id.isEmpty()) {
  //  features->emplace(StringUtil::format("q_cat3:$0", cat3_id.get()));
  //}

  /* cross cat2 x doc id */
  if (!cat2_id.isEmpty()) {
    features->emplace(StringUtil::format(
        "docid:$0|q_cat2:$1",
        docid.docid,
        cat2_id.get()));
  }

  /* cross cat1 id x shop id */
  //if (!cat1_id.isEmpty() && !shop_id.isEmpty()) {
  //  features->emplace(StringUtil::format(
  //      "q_cat1:$0|shop_id:$1",
  //      cat1_id.get(),
  //      shop_id.get()));
  //}

  /* cross cat2 id x shop id */
  if (!cat2_id.isEmpty() && !shop_id.isEmpty()) {
    features->emplace(StringUtil::format(
        "q_cat2:$0|shop_id:$1",
        cat2_id.get(),
        shop_id.get()));
  }

  /* cross cat3 id x shop id */
  //if (!cat3_id.isEmpty() && !shop_id.isEmpty()) {
  //  features->emplace(StringUtil::format(
  //      "q_cat3:$0|shop_id:$1",
  //      cat2_id.get(),
  //      shop_id.get()));
  //}

  /* cross title terms X cat id */
  auto title_de = feature_index_->getFeature(docid, "title~de");
  if (!title_de.isEmpty()) {
    Set<String> terms;
    analyzer_->extractTerms(Language::DE, title_de.get(), &terms);

    for (const auto& term : terms) {
      features->emplace(StringUtil::format("title_term~de:$0", term));

      //if (!cat1_id.isEmpty()) {
      //  features->emplace(StringUtil::format(
      //      "q_cat1:$0|title_term~de:$1",
      //      cat1_id.get(),
      //      term));
      //}

      if (!cat2_id.isEmpty()) {
        features->emplace(StringUtil::format(
            "q_cat2:$0|title_term~de:$1",
            cat2_id.get(),
            term));
      }

      //if (!cat3_id.isEmpty()) {
      //  features->emplace(StringUtil::format(
      //      "q_cat3:$0|title_term~de:$1",
      //      cat3_id.get(),
      //      term));
      //}
    }
  }
}

} // namespace zbase

