/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/net/mysql/MySQL.h>
#include <mutex>

namespace stx {
namespace mysql {

void mysqlInit() {
#ifdef STX_ENABLE_MYSQL
  static std::mutex global_mysql_init_lock;
  static bool global_mysql_initialized = false;

  global_mysql_init_lock.lock();
  if (!global_mysql_initialized) {
    mysql_library_init(0, NULL, NULL); // FIXPAUl mysql_library_end();
    global_mysql_initialized = true;
  }
  global_mysql_init_lock.unlock();
#endif
}

}
}
