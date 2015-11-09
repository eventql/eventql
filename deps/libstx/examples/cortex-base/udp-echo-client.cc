#include <cortex-base/Application.h>
#include <cortex-base/net/UdpClient.h>
#include <cortex-base/net/IPAddress.h>
#include <cortex-base/Buffer.h>
#include <cortex-base/logging.h>

using namespace cortex;

int main(int argc, const char* argv[]) {
  Application::init();
  Application::logToStderr(LogLevel::Trace);

  UdpClient echoClient(IPAddress("127.0.0.1"), 3333);
  echoClient.send("Hello, World");

  Buffer response;
  echoClient.receive(&response);

  logInfo("echo", "received message: \"%*s\"", response.size(), response.data());
}
