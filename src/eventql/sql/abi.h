/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
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
