#ifndef _CM_JOINEDSESSIONVIEWER_H
#define _CM_JOINEDSESSIONVIEWER_H
#include <stx/stdtypes.h>
#include <stx/util/Base64.h>
#include "stx/http/httpservice.h"
#include "common.h"


using namespace stx;
namespace zbase {

class JoinedSessionViewer : public stx::http::HTTPService {
public:

  void handleHTTPRequest(
      stx::http::HTTPRequest* req,
      stx::http::HTTPResponse* res);

};

}
#endif
