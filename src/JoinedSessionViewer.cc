#include "JoinedSessionViewer.h"

using namespace fnord;
namespace cm {

void JoinedSessionViewer::handleHTTPRequest(
  fnord::http::HTTPRequest* req,
  fnord::http::HTTPResponse* res) {

  if (req->method() == http::HTTPMessage::M_GET) {
    URI uri(req->uri());
    fnord::URI::ParamList params = uri.queryParams();

    std::string session;
    if (!fnord::URI::getParam(params, "session", &session)) {
      res->addBody("error: missing session parameter");
      res->setStatus(fnord::http::kStatusBadRequest);
      return;
    }

    fnord::iputs("session parameter: $0", session);
  }

  if (req->method() == http::HTTPMessage::M_POST) {
    res->setStatus(fnord::http::kStatusFound);
    res->addHeader("Location", "/view_session");

    Buffer body = req->body();
    res->addBody(body);
  }
}

}
