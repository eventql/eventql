/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "DocumentDBServlet.h"
#include "stx/human.h"

using namespace stx;

namespace zbase {

DocumentDBServlet::DocumentDBServlet(
    DocumentDB* docdb) :
    docdb_(docdb) {}

void DocumentDBServlet::handle(
    const AnalyticsSession& session,
    const stx::http::HTTPRequest* req,
    stx::http::HTTPResponse* res) {
  static const String kPathPrefix = "/api/v1/documents";
  URI uri(req->uri());

  // LIST
  if (req->method() == http::HTTPMessage::M_GET &&
      uri.path() == kPathPrefix) {
    listDocuments(session, req, res);
    return;
  }

  // GET
  if (req->method() == http::HTTPMessage::M_GET &&
      StringUtil::beginsWith(uri.path(), kPathPrefix)) {
    auto docid = SHA1Hash::fromHexString(
        uri.path().substr(kPathPrefix.size() + 1));

    fetchDocument(docid, uri, session, req, res);
    return;
  }

  // CREATE
  if (req->method() == http::HTTPMessage::M_POST &&
      uri.path() == kPathPrefix) {
    createDocument(uri, session, req, res);
    return;
  }

  // UPDATE
  if (req->method() == http::HTTPMessage::M_POST &&
      StringUtil::beginsWith(uri.path(), kPathPrefix)) {
    auto docid = SHA1Hash::fromHexString(
        uri.path().substr(kPathPrefix.size() + 1));

    updateDocument(docid, uri, session, req, res);
    return;
  }

  res->setStatus(stx::http::kStatusNotFound);
  res->addBody("not found");
}

void DocumentDBServlet::fetchDocument(
    const SHA1Hash& uuid,
    const URI& uri,
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  const auto& params = uri.queryParams();

  bool return_content = true;
  String no_content_str;
  if (URI::getParam(params, "no_content", &no_content_str)) {
    return_content = false;
  }

  Document doc;
  if (docdb_->fetchDocument(session.customer(), session.userid(), uuid, &doc)) {
    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
    renderDocument(session, doc, &json, return_content);
    res->setStatus(http::kStatusOK);
    res->addBody(buf);
  } else {
    res->setStatus(http::kStatusNotFound);
    res->addBody("not found");
  }
}

void DocumentDBServlet::createDocument(
    const URI& uri,
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  URI::ParamList params;
  URI::parseQueryString(req->body().toString(), &params);

  String name;
  if (!URI::getParam(params, "name", &name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?name=... parameter");
  }

  String type;
  if (!URI::getParam(params, "type", &type)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?type=... parameter");
  }

  auto now = WallClock::unixMicros();

  Document doc;
  doc.set_uuid(Random::singleton()->sha1().toString());
  doc.set_name(name);
  doc.set_type(type);
  doc.set_version(0);
  doc.set_ctime(now);
  doc.set_mtime(now);
  doc.set_atime(now);

  setDefaultDocumentACLs(&doc, session.userid());

  docdb_->createDocument(session.customer(), doc);

  Buffer buf;
  json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
  renderDocument(session, doc, &json);
  res->setStatus(http::kStatusCreated);
  res->addBody(buf);
}

void DocumentDBServlet::updateDocument(
    const SHA1Hash& uuid,
    const URI& uri,
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  URI::ParamList params;
  URI::parseQueryString(req->body().toString(), &params);

  Vector<Function<void ()>> tx;

  for (const auto& p : params) {

    if (p.first == "content") {
      if (!p.second.empty()) {
        tx.emplace_back([this, &session, &uuid, p] () {
          docdb_->updateDocumentContent(
              session.customer(),
              session.userid(),
              uuid,
              p.second);
        });
      }

      continue;
    }

    if (p.first == "name") {
      if (!p.second.empty()) {
        tx.emplace_back([this, &session, &uuid, p] () {
          docdb_->updateDocumentName(
              session.customer(),
              session.userid(),
              uuid,
              p.second);
        });
      }

      continue;
    }

    if (p.first == "category") {
      if (!p.second.empty()) {
        tx.emplace_back([this, &session, &uuid, p] () {
          docdb_->updateDocumentCategory(
              session.customer(),
              session.userid(),
              uuid,
              p.second);
        });
      }

      continue;
    }

    if (p.first == "acl_policy") {
      if (!p.second.empty()) {
        DocumentACLPolicy policy;
        if (!DocumentACLPolicy_Parse(p.second, &policy)) {
          res->setStatus(http::kStatusBadRequest);
          res->addBody(StringUtil::format("invalid policy: '$0'", p.second));
          return;
        }

        tx.emplace_back([this, &session, &uuid, policy] () {
          docdb_->updateDocumentACLPolicy(
              session.customer(),
              session.userid(),
              uuid,
              policy);
        });
      }

      continue;
    }

    if (p.first == "publishing_status") {
      if (!p.second.empty()) {
        DocumentPublishingStatus pstatus;
        if (!DocumentPublishingStatus_Parse(p.second, &pstatus)) {
          res->setStatus(http::kStatusBadRequest);
          res->addBody(
              StringUtil::format("invalid publishing status: '$0'", p.second));
          return;
        }

        tx.emplace_back([this, &session, &uuid, pstatus] () {
          docdb_->updateDocumentPublishingStatus(
              session.customer(),
              session.userid(),
              uuid,
              pstatus);
        });
      }

      continue;
    }

    if (p.first == "deleted") {
      if (!p.second.empty()) {
        auto deleted = Human::parseBoolean(p.second);
        if (deleted.isEmpty()) {
          res->setStatus(http::kStatusBadRequest);
          res->addBody(
              StringUtil::format("invalid deleted status: '$0'", p.second));
          return;
        }

        tx.emplace_back([this, &session, &uuid, deleted] () {
          docdb_->updateDocumentDeletedStatus(
              session.customer(),
              session.userid(),
              uuid,
              deleted.get());
        });
      }

      continue;
    }


    res->setStatus(http::kStatusBadRequest);
    res->addBody(StringUtil::format("invalid parameter: '$0'", p.first));
    return;
  }

  for (const auto& fn : tx) {
    fn();
  }

  res->setStatus(http::kStatusCreated);
}

void DocumentDBServlet::listDocuments(
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  URI uri(req->uri());
  const auto& params = uri.queryParams();

  /* param: with_categories */
  Set<String> categories;
  bool return_categories = false;
  String with_categories_str;
  if (URI::getParam(params, "with_categories", &with_categories_str)) {
    return_categories = true;
  }

  /* param: publishing_status */
  Option<DocumentPublishingStatus> pstatus_filter;
  String pstatus_filter_str;
  if (URI::getParam(params, "publishing_status", &pstatus_filter_str)) {
    DocumentPublishingStatus pstatus;
    if (!DocumentPublishingStatus_Parse(pstatus_filter_str, &pstatus)) {
      res->setStatus(http::kStatusBadRequest);
      res->addBody(StringUtil::format(
          "invalid publishing status: '$0'",
          pstatus_filter_str));
      return;
    }

    pstatus_filter = Some(pstatus);
  }

  /* param: category_prefix */
  String category_prefix_filter;
  URI::getParam(params, "category_prefix", &category_prefix_filter);

  /* scan documents */
  Buffer buf;
  json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));

  json.beginObject();
  json.addObjectEntry("documents");
  json.beginArray();

  size_t num_docs = 0;
  size_t num_docs_total = 0;
  size_t num_docs_user = 0;
  docdb_->listDocuments(
      session.customer(),
      session.userid(),
      [&] (const Document& doc) -> bool {
    if (isDocumentReadableForUser(doc, session.userid())) {
      ++num_docs_total;
      ++num_docs_user;
    }

    if (!doc.category().empty()) {
      categories.emplace(doc.category());
    }

    if (!pstatus_filter.isEmpty() &&
        doc.publishing_status() != pstatus_filter.get()) {
      return true;
    }

    if (!category_prefix_filter.empty() &&
        !StringUtil::beginsWith(doc.category(), category_prefix_filter)) {
      return true;
    }

    if (++num_docs > 1) {
      json.addComma();
    }

    renderDocument(
        session,
        doc,
        &json,
        false);

    return true;
  });

  json.endArray();
  json.addComma();

  json.addObjectEntry("num_docs");
  json.addInteger(num_docs);
  json.addComma();

  json.addObjectEntry("num_docs_total");
  json.addInteger(num_docs_total);
  json.addComma();

  json.addObjectEntry("num_docs_user");
  json.addInteger(num_docs_user);

  if (return_categories) {
    json.addComma();
    json.addObjectEntry("categories");
    json::toJSON(categories, &json);
  }

  json.endObject();

  res->setStatus(http::kStatusOK);
  res->setHeader("Content-Type", "application/json; charset=utf-8");
  res->addBody(buf);
}

void DocumentDBServlet::renderDocument(
    const AnalyticsSession& session,
    const Document& doc,
    json::JSONOutputStream* json,
    bool return_content /* = true */) {
  json->beginObject();

  json->addObjectEntry("uuid");
  json->addString(doc.uuid());
  json->addComma();

  json->addObjectEntry("name");
  json->addString(doc.name());
  json->addComma();

  json->addObjectEntry("type");
  json->addString(doc.type());
  json->addComma();

  json->addObjectEntry("publishing_status");
  json->addString(DocumentPublishingStatus_Name(doc.publishing_status()));
  json->addComma();

  json->addObjectEntry("category");
  json->addString(doc.category());
  json->addComma();

  json->addObjectEntry("acl_policy");
  json->addString(DocumentACLPolicy_Name(doc.acl_policy()));
  json->addComma();

  json->addObjectEntry("is_readable");
  json->addBool(isDocumentReadableForUser(doc, session.userid()));
  json->addComma();

  json->addObjectEntry("is_writable");
  json->addBool(isDocumentWritableForUser(doc, session.userid()));
  json->addComma();

  json->addObjectEntry("deleted");
  json->addBool(doc.deleted());
  json->addComma();

  json->addObjectEntry("ctime");
  json->addInteger(doc.ctime());
  json->addComma();

  json->addObjectEntry("mtime");
  json->addInteger(doc.mtime());
  json->addComma();

  json->addObjectEntry("atime");
  json->addInteger(doc.atime());

  if (return_content || doc.type() == "application") {
    json->addComma();
    json->addObjectEntry("content");
    json->addString(doc.content());
  }

  json->endObject();
}

} // namespace zbase
