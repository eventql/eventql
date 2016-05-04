#ifndef _CM_JOINEDSESSIONVIEWER_H
#define _CM_JOINEDSESSIONVIEWER_H
#include <eventql/util/stdtypes.h>
#include <eventql/util/util/Base64.h>
#include "eventql/util/http/httpservice.h"
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
