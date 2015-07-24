#ifndef _CM_JOINEDSESSIONVIEWER_H
#define _CM_JOINEDSESSIONVIEWER_H
#include <fnord/stdtypes.h>
#include <fnord/util/Base64.h>
#include "fnord/http/httpservice.h"
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
