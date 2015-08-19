/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_SQL_MYSQL_H
#define _STX_SQL_MYSQL_H
#ifdef STX_ENABLE_MYSQL
#include <mysql.h>
#endif

namespace stx {
namespace mysql {

void mysqlInit();

}
}
#endif
