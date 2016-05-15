/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#include <stdlib.h>
#include <stdint.h>

#if defined _WIN32 || defined __CYGWIN__
  #ifdef BUILDING_DLL
    #ifdef __GNUC__
      #define SQLAPI_PUBLIC __attribute__ ((dllexport))
    #else
      #define SQLAPI_PUBLIC __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #else
    #ifdef __GNUC__
      #define SQLAPI_PUBLIC __attribute__ ((dllimport))
    #else
      #define SQLAPI_PUBLIC __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #endif
#else
  #if __GNUC__ >= 4
    #define SQLAPI_PUBLIC __attribute__ ((visibility ("default")))
  #else
    #define SQLAPI_PUBLIC
  #endif
#endif

extern "C" {

enum sql_type : uint8_t {
  SQL_NULL = 0,
  SQL_STRING = 1,
  SQL_FLOAT = 2,
  SQL_INTEGER = 3,
  SQL_BOOL = 4,
  SQL_TIMESTAMP = 5,
};

struct sql_txn__ { int unused; };
typedef struct sql_txn__* sql_txn;

struct sql_val__ { int unused; };
typedef struct sql_val__* sql_val;

struct sql_args__ { int unused; };
typedef struct sql_val__* sql_args;

SQLAPI_PUBLIC sql_val* sql_getarg(sql_args* args, size_t idx);

SQLAPI_PUBLIC bool sql_getint(sql_val* in, int64_t* out);
SQLAPI_PUBLIC bool sql_getfloat(sql_val* in, double* out);
SQLAPI_PUBLIC bool sql_getstring(sql_val* in, const char** data, size_t* size);
SQLAPI_PUBLIC bool sql_getbool(sql_val* in, bool* out);

};
