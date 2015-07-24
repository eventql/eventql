#ifndef _CM_JOINEDSESSIONVIEWER_H
#define _CM_JOINEDSESSIONVIEWER_H
#include <stx/stdtypes.h>
#include <stx/util/Base64.h>
#include "stx/http/httpservice.h"
#include "common.h"


using namespace fnord;
namespace cm {

class JoinedSessionViewer : public fnord::http::HTTPService {
public:

  void handleHTTPRequest(
      fnord::http::HTTPRequest* req,
      fnord::http::HTTPResponse* res);

};

}
#endif
