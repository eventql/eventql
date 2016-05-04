/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "IndexServlet.h"
#include "SearchQuery.h"
#include "eventql/util/json/json.h"

using namespace stx;

namespace zbase {

IndexServlet::IndexServlet(
    RefPtr<zbase::IndexReader> index,
    RefPtr<fts::Analyzer> analyzer) :
    index_(index),
    analyzer_(analyzer) {}

void IndexServlet::handleHTTPRequest(
    stx::http::HTTPRequest* req,
    stx::http::HTTPResponse* res) {
  URI uri(req->uri());

  res->addHeader("Access-Control-Allow-Origin", "*");

  try {
    if (StringUtil::endsWith(uri.path(), "/search")) {
      return searchQuery(req, res, &uri);
    }

    if (StringUtil::endsWith(uri.path(), "/doc")) {
      return fetchDoc(req, res, &uri);
    }

    if (StringUtil::endsWith(uri.path(), "/docs")) {
      return fetchDocs(req, res, &uri);
    }

    res->setStatus(stx::http::kStatusNotFound);
    res->addBody("not found");
  } catch (const Exception& e) {
    res->setStatus(http::kStatusInternalServerError);
    res->addBody(StringUtil::format("error: $0", e.getMessage()));
  }
}

void IndexServlet::searchQuery(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String query_str;
  if (!stx::URI::getParam(params, "q", &query_str)) {
    res->addBody("error: missing ?q=... parameter");
    res->setStatus(http::kStatusBadRequest);
    return;
  }

  SearchQuery query;
  query.addField("cm_clicked_terms", 2.5);
  query.addField("title~de", 1.5);
  query.addField("tags", 1.1);
  query.addField("text~de", 1.0);
  query.addQuery(query_str, Language::DE, analyzer_.get());
  query.execute(index_.get());

  res->setStatus(http::kStatusOK);
  res->addHeader("Content-Type", "application/json; charset=utf-8");
  json::JSONOutputStream jsons(res->getBodyOutputStream());
  query.writeJSON(&jsons);
}

void IndexServlet::fetchDoc(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String docid_str;
  if (!stx::URI::getParam(params, "id", &docid_str)) {
    res->addBody("error: missing ?id=... parameter");
    res->setStatus(http::kStatusBadRequest);
    return;
  }

  DocID docid = { .docid = docid_str };
  auto doc = index_->docIndex()->findDocument(docid);

  auto fields = doc->fields();
  fields["docid"] = docid_str;

  auto doc_json = json::toJSONString(fields);
  res->setStatus(http::kStatusOK);
  res->addHeader("Content-Type", "application/json; charset=utf-8");
  res->addBody(doc_json);
}

void IndexServlet::fetchDocs(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String docids_str;
  if (!stx::URI::getParam(params, "ids", &docids_str)) {
    res->addBody("error: missing ?ids=... parameter");
    res->setStatus(http::kStatusBadRequest);
    return;
  }

  auto docid_strs = StringUtil::split(docids_str, ",");

  Set<String> field_list;
  String fieldlist_str;
  if (stx::URI::getParam(params, "fields", &fieldlist_str)) {
    for (const auto& f : StringUtil::split(fieldlist_str, ",")) {
      field_list.emplace(f);
    }
  }

  json::JSONObject json;
  json.emplace_back(json::JSON_OBJECT_BEGIN);

  for (const auto& docid_str : docid_strs) {
    DocID docid = { .docid = docid_str };
    auto doc = index_->docIndex()->findDocument(docid);

    auto fields = doc->fields();
    fields["docid"] = docid_str;
    if (field_list.size() > 0) {
      for (auto iter = fields.begin(); iter != fields.end(); ) {
        if (field_list.count(iter->first) == 0) {
          iter = fields.erase(iter);
        } else {
          ++iter;
        }
      }
    }

    json.emplace_back(json::JSON_STRING, docid_str);
    json::toJSON(fields, &json);
  }

  json.emplace_back(json::JSON_OBJECT_END);

  auto doc_json = json::toJSONString(json);
  res->setStatus(http::kStatusOK);
  res->addHeader("Content-Type", "application/json; charset=utf-8");
  res->addBody(doc_json);
}

}
