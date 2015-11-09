// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <cortex-base/net/DnsClient.h>
#include <cortex-base/cli/CLI.h>
#include <cortex-base/cli/Flags.h>
#include <cortex-base/logging.h>
#include <iostream>

using namespace cortex;

int main(int argc, const char* argv[]) {
  try {
    DnsClient dns;
    CLI cli;
    cli.defineBool("ipv6", '6', "Enable resolving IPv6.");
    cli.defineBool("help", 'h', "Shows this help and terminates.");
    cli.enableParameters("<hostnames>", "Hostnames to resolve.");

    Flags flags = cli.evaluate(argc, argv);

    if (flags.getBool("help")) {
      std::cout << "CLI:" << std::endl
                << cli.helpText() << std::endl;
      return 0;
    }

    std::vector<std::string> hosts = flags.parameters();
    if (hosts.empty()) {
      std::cerr << "No hosts given." << std::endl;
      return 1;
    }

    for (const std::string& host: hosts) {
      printf("resolving '%s'\n", host.c_str());
      std::vector<IPAddress> ips;
      if (flags.getBool("ipv6")) {
        ips = dns.ipv6(host);
      } else {
        ips = dns.ipv4(host);
      }
      printf("  found IPs: %zu\n", ips.size());
      for (const IPAddress& ip: ips) {
        printf("    ip: %s\n", ip.c_str());
      }
    }
  } catch (const std::exception& e) {
    logError("example", e);
  }

  return 0;
}
