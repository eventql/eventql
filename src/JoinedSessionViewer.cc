#include "JoinedSessionViewer.h"

using namespace fnord;
namespace cm {

void JoinedSessionViewer::handleHTTPRequest(
  fnord::http::HTTPRequest* req,
  fnord::http::HTTPResponse* res) {

  if (req->method() == http::HTTPMessage::M_GET) {
    fnord::iputs("get $0", 1);
  }

  if (req->method() == http::HTTPMessage::M_POST) {
    res->setStatus(fnord::http::kStatusFound);
    res->addHeader("Location", "/view_session");
  }
}

}
