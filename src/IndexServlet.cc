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
#include "fnord-json/json.h"

using namespace fnord;

namespace cm {

IndexServlet::IndexServlet(
    RefPtr<cm::IndexReader> index,
    RefPtr<fts::Analyzer> analyzer) :
    index_(index),
    analyzer_(analyzer) {}

void IndexServlet::handleHTTPRequest(
    fnord::http::HTTPRequest* req,
    fnord::http::HTTPResponse* res) {
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

    res->setStatus(fnord::http::kStatusNotFound);
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
  if (!fnord::URI::getParam(params, "q", &query_str)) {
    res->addBody("error: missing ?q=... parameter");
    res->setStatus(http::kStatusBadRequest);
    return;
  }

  SearchQuery query;
  query.addField("title~de", 2.0);
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
  if (!fnord::URI::getParam(params, "id", &docid_str)) {
    res->addBody("error: missing ?id=... parameter");
    res->setStatus(http::kStatusBadRequest);
    return;
  }

  DocID docid = { .docid = docid_str };
  auto txn = index_->db_->startTransaction(true);
  auto doc = index_->docIndex()->findDocument(docid, txn.get());
  txn->abort();

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
  if (!fnord::URI::getParam(params, "ids", &docids_str)) {
    res->addBody("error: missing ?ids=... parameter");
    res->setStatus(http::kStatusBadRequest);
    return;
  }

  auto docid_strs = StringUtil::split(docids_str, ",");

  json::JSONObject json;
  json.emplace_back(json::JSON_OBJECT_BEGIN);

  for (const auto& docid_str : docid_strs) {
    DocID docid = { .docid = docid_str };
    auto txn = index_->db_->startTransaction(true);
    auto doc = index_->docIndex()->findDocument(docid, txn.get());
    txn->abort();

    auto fields = doc->fields();
    fields["docid"] = docid_str;

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
