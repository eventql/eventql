// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-http/fastcgi/Generator.h>
#include <cortex-base/net/EndPointWriter.h>
#include <cortex-base/net/ByteArrayEndPoint.h>
#include <cortex-base/logging.h>
#include <gtest/gtest.h>

using namespace cortex;
using namespace cortex::http;

TEST(http_fastcgi_Generator, simpleRequest) {
  constexpr BufferRef content = "hello, world";
  HeaderFieldList headers = {
    {"Foo", "the-foo"},
    {"Bar", "the-bar"}
  };
  HttpRequestInfo info(HttpVersion::VERSION_1_1, "PUT", "/index.html",
                       content.size(), headers);

  EndPointWriter writer;
  http::fastcgi::Generator generator(1, &writer);
  generator.generateRequest(info);
  generator.generateBody(content);
  generator.generateEnd();

  ByteArrayEndPoint ep(nullptr);
  writer.flush(&ep);

  //printf("%s\n", ep.output().hexdump(HexDumpMode::PrettyAscii).c_str());

  //...
}

TEST(http_fastcgi_Generator, simpleResponse) {
  BufferRef content = "hello, world";
  HeaderFieldList headers = {
    {"Foo", "the-foo"},
    {"Bar", "the-bar"}
  };
  HttpResponseInfo info(HttpVersion::VERSION_1_1, HttpStatus::Ok, "my",
      false, content.size(), headers, {});

  EndPointWriter writer;
  http::fastcgi::Generator generator(1, &writer);
  generator.generateResponse(info);
  generator.generateBody(content);
  generator.generateEnd();

  ByteArrayEndPoint ep(nullptr);
  writer.flush(&ep);

  //printf("%s\n", ep.output().hexdump(HexDumpMode::PrettyAscii).c_str());

  // TODO: verify
}
