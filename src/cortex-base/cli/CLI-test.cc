// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/cli/CLI.h>
#include <cortex-base/cli/Flags.h>
#include <cortex-base/net/IPAddress.h>
#include <gtest/gtest.h>

using namespace cortex;

/**
 * cmdline test cases
 *
 * - explicit boolean (long option)
 * - explicit boolean (short option)
 * - explicit short option, single
 * - explicit short option, first in chain
 * - explicit short option, deep in chain
 * - defaulting long-option
 * - required long-option with value
 * - required short-option with value
 * - errors
 *   - raise on unknown long option
 *   - raise on unknown short option
 *   - raise on missing long-option value
 *   - raise on missing short-option value
 *   - raise on missing required option
 *   - raise on invalid type (int)
 *   - raise on invalid type (float)
 *   - raise on invalid type (ip)
 */

TEST(CLI, defaults) {
  CLI cli;
  cli.defineString("some", 's', "<text>", "Something", "some value", nullptr);
  cli.defineBool("bool", 'b', "some boolean");

  Flags flags = cli.evaluate({});

  ASSERT_EQ(2, flags.size());
  ASSERT_EQ("some value", flags.getString("some"));
  ASSERT_EQ(false, flags.getBool("bool"));
}

TEST(CLI, raise_on_unknown_long_option) {
  CLI cli;
  cli.defineBool("some", 's', "Something");
  ASSERT_THROW(cli.evaluate({"--something-else"}), CLI::UnknownOptionError);
}

TEST(CLI, raise_on_unknown_short_option) {
  CLI cli;
  cli.defineBool("some", 's', "Something");
  ASSERT_THROW(cli.evaluate({"-t"}), CLI::UnknownOptionError);
}

TEST(CLI, raise_on_missing_long_option) {
  CLI cli;
  cli.defineString("some", 's', "<text>", "Something");
  ASSERT_THROW(cli.evaluate({"--some"}), CLI::MissingOptionValueError);
}

TEST(CLI, raise_on_missing_option_value) {
  CLI cli;
  cli.defineString("some", 's', "<some>", "Something");
  cli.defineString("tea", 't', "<some>", "Tea Time");
  ASSERT_THROW(cli.evaluate({"-s", "-tblack"}), CLI::MissingOptionValueError);
  ASSERT_THROW(cli.evaluate({"-swhite", "-t"}), CLI::MissingOptionValueError);
}

TEST(CLI, short_option_values) {
  CLI cli;
  cli.defineString("some", 's', "<text>", "Something");
  cli.defineString("tea", 't', "<text>", "Tea Time");

  Flags flags = cli.evaluate({"-sthing", "-t", "time"});

  ASSERT_EQ(2, flags.size());
  ASSERT_EQ("thing", flags.getString("some"));
  ASSERT_EQ("time", flags.getString("tea"));
}

TEST(CLI, short_option_single) {
  CLI cli;
  cli.defineBool("some", 's', "Something");

  Flags flags = cli.evaluate({"-s"});

  ASSERT_EQ(1, flags.size());
  ASSERT_TRUE(flags.getBool("some"));
}

TEST(CLI, short_option_multi) {
  CLI cli;
  cli.defineBool("some", 's', "The Some");
  cli.defineBool("thing", 't', "The Thing");
  cli.defineBool("else", 'e', "The Else");

  Flags flags = cli.evaluate({"-tes"});

  ASSERT_EQ(3, flags.size());
  ASSERT_TRUE(flags.getBool("some"));
  ASSERT_TRUE(flags.getBool("thing"));
  ASSERT_TRUE(flags.getBool("else"));
}

TEST(CLI, short_option_multi_mixed) {
  CLI cli;
  cli.defineBool("some", 's', "The Some");
  cli.defineString("text", 't', "<text>", "The Text");

  Flags flags = cli.evaluate({"-sthello"});

  ASSERT_EQ(2, flags.size());
  ASSERT_TRUE(flags.getBool("some"));
  ASSERT_EQ("hello", flags.getString("text"));
}

TEST(CLI, short_option_value_inline) {
  CLI cli;
  cli.defineString("text", 't', "<text>", "The Text");

  Flags flags = cli.evaluate({"-thello"});

  ASSERT_EQ(1, flags.size());
  ASSERT_EQ("hello", flags.getString("text"));
}

TEST(CLI, short_option_value_sep) {
  CLI cli;
  cli.defineString("text", 't', "<text>", "The Text");

  Flags flags = cli.evaluate({"-t", "hello"});

  ASSERT_EQ(1, flags.size());
  ASSERT_EQ("hello", flags.getString("text"));
}

TEST(CLI, long_option_with_value_inline) {
  CLI cli;
  cli.defineString("text", 't', "<text>", "The Text");

  Flags flags = cli.evaluate({"--text=hello"});

  ASSERT_EQ(1, flags.size());
  ASSERT_EQ("hello", flags.getString("text"));
}

TEST(CLI, long_option_with_value_sep) {
  CLI cli;
  cli.defineString("text", 't', "<text>", "The Text");

  Flags flags = cli.evaluate({"--text", "hello"});

  ASSERT_EQ(1, flags.size());
  ASSERT_EQ("hello", flags.getString("text"));
}

TEST(CLI, type_int) {
  CLI cli;
  cli.defineNumber("number", 'n', "<number>", "The Number");

  Flags flags = cli.evaluate({"-n42"});
  ASSERT_EQ(1, flags.size());
  ASSERT_EQ(42, flags.getNumber("number"));
}

TEST(CLI, type_float) {
  CLI cli;
  cli.defineFloat("float", 'f', "<float>", "The Float");

  Flags flags = cli.evaluate({"-f1.42"});
  ASSERT_EQ(1, flags.size());
  ASSERT_EQ(1.42f, flags.getFloat("float"));
}

TEST(CLI, type_ip) {
  CLI cli;
  cli.defineIPAddress("ip", 'a', "<IP>", "The IP");

  Flags flags = cli.evaluate({"--ip=4.2.2.1"});
  ASSERT_EQ(1, flags.size());
  ASSERT_EQ(IPAddress("4.2.2.1"), flags.getIPAddress("ip"));
}

TEST(CLI, callbacks_on_explicit) {
  IPAddress bindIP;

  CLI cli;
  cli.defineIPAddress("bind", 'a', "<IP>", "IP address to bind listener address to.",
      [&](const IPAddress& ip) {
        bindIP = ip;
      });

  cli.evaluate({"--bind", "127.0.0.2"});

  ASSERT_EQ(IPAddress("127.0.0.2"), bindIP);
}

TEST(CLI, callbacks_on_defaults) {
  IPAddress bindIP;

  CLI cli;
  cli.defineIPAddress(
      "bind", 'a', "<IP>", "IP address to bind listener address to.",
      IPAddress("127.0.0.2"),
      [&](const IPAddress& ip) { bindIP = ip; });

  cli.evaluate({"--bind", "127.0.0.3"});
  ASSERT_EQ(IPAddress("127.0.0.3"), bindIP);

  cli.evaluate({});
  ASSERT_EQ(IPAddress("127.0.0.2"), bindIP);
}

TEST(CLI, callbacks_on_repeated_args) {
  std::vector<IPAddress> hosts;
  CLI cli;
  cli.defineIPAddress(
      "host", 't', "<IP>", "Host address to talk to.",
      [&](const IPAddress& host) { hosts.emplace_back(host); });

  cli.evaluate({"--host=127.0.0.1", "--host=192.168.0.1", "-t10.10.20.40"});

  ASSERT_EQ(3, hosts.size());
  ASSERT_EQ(IPAddress("127.0.0.1"), hosts[0]);
  ASSERT_EQ(IPAddress("192.168.0.1"), hosts[1]);
  ASSERT_EQ(IPAddress("10.10.20.40"), hosts[2]);
}

TEST(CLI, argc_argv_to_vector) {
  CLI cli;
  cli.defineBool("help", 'h', "Shows this help and terminates.");
  cli.defineBool("bool", 'b', "some boolean");

  const int argc = 3;
  static const char* argv[] = {
    "/proc/self/exe",
    "--help",
    "-b",
    nullptr
  };

  Flags flags = cli.evaluate(argc, argv);

  ASSERT_EQ(2, flags.size());
  ASSERT_TRUE(flags.getBool("help"));
  ASSERT_TRUE(flags.getBool("bool"));
}
