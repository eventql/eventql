#ifndef _CM_JOINEDSESSIONVIEWER_H
#define _CM_JOINEDSESSIONVIEWER_H
#include "fnord-http/httpservice.h"

namespace cm {

class JoinedSessionViewer : public fnord::http::HTTPService {
public:

  void handleHTTPRequest(
      fnord::http::HTTPRequest* req,
      fnord::http::HTTPResponse* res);

};

}
#endif
