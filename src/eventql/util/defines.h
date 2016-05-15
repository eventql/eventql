/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *   - Christian Parpart <trapni@dawanda.com>
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

#ifndef libstx_defines_h
#define libstx_defines_h (1)

#include <eventql/sysconfig.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

// platforms
#if defined(_WIN32) || defined(__WIN32__)
#define STX_OS_WIN32 1
//#	define _WIN32_WINNT 0x0510
#else
#define STX_OS_UNIX 1
#if defined(__CYGWIN__)
#define STX_OS_WIN32 1
#elif defined(__APPLE__)
#define STX_OS_DARWIN 1 /* MacOS/X 10 */
#endif
#endif

// api decl tools
#if defined(__GNUC__)
#define STX_NO_EXPORT __attribute__((visibility("hidden")))
#define STX_EXPORT __attribute__((visibility("default")))
#define STX_IMPORT /*!*/
#define STX_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#define STX_NO_RETURN __attribute__((no_return))
#define STX_DEPRECATED __attribute__((__deprecated__))
#define STX_PURE __attribute__((pure))
#define STX_PACKED __attribute__((packed))
#define STX_INIT __attribute__((constructor))
#define STX_FINI __attribute__((destructor))
#if !defined(likely)
#define likely(x) __builtin_expect((x), 1)
#endif
#if !defined(unlikely)
#define unlikely(x) __builtin_expect((x), 0)
#endif
#elif defined(__MINGW32__)
#define STX_NO_EXPORT /*!*/
#define STX_EXPORT __declspec(export)
#define STX_IMPORT __declspec(import)
#define STX_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#define STX_NO_RETURN __attribute__((no_return))
#define STX_DEPRECATED __attribute__((__deprecated__))
#define STX_PURE __attribute__((pure))
#define STX_PACKED __attribute__((packed))
#define STX_INIT /*!*/
#define STX_FINI /*!*/
#if !defined(likely)
#define likely(x) (x)
#endif
#if !defined(unlikely)
#define unlikely(x) (x)
#endif
#elif defined(__MSVC__)
#define STX_NO_EXPORT /*!*/
#define STX_EXPORT __declspec(export)
#define STX_IMPORT __declspec(import)
#define STX_WARN_UNUSED_RESULT /*!*/
#define STX_NO_RETURN          /*!*/
#define STX_DEPRECATED         /*!*/
#define STX_PURE               /*!*/
#define STX_PACKED __packed    /* ? */
#define STX_INIT               /*!*/
#define STX_FINI               /*!*/
#if !defined(likely)
#define likely(x) (x)
#endif
#if !defined(unlikely)
#define unlikely(x) (x)
#endif
#else
#warning Unknown platform
#define STX_NO_EXPORT          /*!*/
#define STX_EXPORT             /*!*/
#define STX_IMPORT             /*!*/
#define STX_WARN_UNUSED_RESULT /*!*/
#define STX_NO_RETURN          /*!*/
#define STX_DEPRECATED         /*!*/
#define STX_PURE               /*!*/
#define STX_PACKED             /*!*/
#define STX_INIT               /*!*/
#define STX_FINI               /*!*/
#if !defined(likely)
#define likely(x) (x)
#endif
#if !defined(unlikely)
#define unlikely(x) (x)
#endif
#endif

#if defined(__GNUC__)
#define GCC_VERSION(major, minor) \
  ((__GNUC__ > (major)) || (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))
#define CC_SUPPORTS_LAMBDA GCC_VERSION(4, 5)
#define CC_SUPPORTS_RVALUE_REFERENCES GCC_VERSION(4, 4)
#else
#define GCC_VERSION(major, minor) (0)
#define CC_SUPPORTS_LAMBDA (0)
#define CC_SUPPORTS_RVALUE_REFERENCES (0)
#endif

/// the filename only part of __FILE__ (no leading path)
#define __FILENAME__ ((std::strrchr(__FILE__, '/') ?: __FILE__ - 1) + 1)

#if defined(STX_ENABLE_NOEXCEPT)
#define STX_NOEXCEPT noexcept
#else
#define STX_NOEXCEPT /*STX_NOEXCEPT*/
#endif

#endif
