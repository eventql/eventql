#include "JoinedSessionViewer.h"

using namespace stx;
namespace zbase {

void JoinedSessionViewer::handleHTTPRequest(
  stx::http::HTTPRequest* req,
  stx::http::HTTPResponse* res) {

  if (req->method() == http::HTTPMessage::M_GET) {
    URI uri(req->uri());
    stx::URI::ParamList params = uri.queryParams();

    std::string session;
    if (!stx::URI::getParam(params, "session", &session)) {
      res->addBody("error: missing session parameter");
      res->setStatus(stx::http::kStatusBadRequest);
      return;
    }

    stx::iputs("session parameter: $0", session);
  }

  if (req->method() == http::HTTPMessage::M_POST) {
    Buffer body = req->body();
    String session_param;
    util::Base64::encode(body.toString(), &session_param);

    //redirect
    res->setStatus(stx::http::kStatusFound);
    res->addHeader("Location", "/view_session" + session_param);
  }
}

}
