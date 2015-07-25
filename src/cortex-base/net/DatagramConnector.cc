// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/net/DatagramConnector.h>

namespace cortex {

DatagramConnector::DatagramConnector(
    const std::string& name,
    DatagramHandler handler,
    Executor* executor)
    : name_(name),
      handler_(handler),
      executor_(executor) {
}

DatagramConnector::~DatagramConnector() {
}

DatagramHandler DatagramConnector::handler() const {
  return handler_;
}

void DatagramConnector::setHandler(DatagramHandler handler) {
  handler_ = handler;
}

} // namespace cortex
