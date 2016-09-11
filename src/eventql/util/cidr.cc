/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#pragma once
#include <string>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <limits.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <netinet/tcp.h>

bool cidr_match(
    const std::string& cidr_range,
    const std::string& ip) {
  auto cidr_mask_pos = cidr_range.find("/");
  if (cidr_mask_pos == std::string::npos) {
    return false;
  }

  auto cidr_ip = cidr_range.substr(0, cidr_mask_pos);
  uint16_t cidr_mask;
  try {
    cidr_mask = std::stoul(cidr_range.substr(cidr_mask_pos + 1));
  } catch (...) {
    return false;
  }

  struct sockaddr_in sip;
  if (inet_pton(AF_INET, ip.c_str(), &(sip.sin_addr)) != 1) {
    return false;
  }

  struct sockaddr_in cidr_sip;
  if (inet_pton(AF_INET, cidr_ip.c_str(), &(cidr_sip.sin_addr)) != 1) {
    return false;
  }

  if (cidr_mask == 0) {
    return true;
  }

  long mask_expaneded = htonl(0xFFFFFFFFu << (32 - cidr_mask));
  return !((sip.sin_addr.s_addr ^ cidr_sip.sin_addr.s_addr) & mask_expaneded);
}

