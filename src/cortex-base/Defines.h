// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#ifndef sw_x0_defines_hpp
#define sw_x0_defines_hpp (1)

#include <cortex-base/sysconfig.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

// platforms
#if defined(_WIN32) || defined(__WIN32__)
#define CORTEX_OS_WIN32 1
//#	define _WIN32_WINNT 0x0510
#else
#define CORTEX_OS_UNIX 1
#if defined(__CYGWIN__)
#define CORTEX_OS_WIN32 1
#elif defined(__APPLE__)
#define CORTEX_OS_DARWIN 1 /* MacOS/X 10 */
#endif
#endif

// api decl tools
#if defined(__GNUC__)
#define CORTEX_NO_EXPORT __attribute__((visibility("hidden")))
#define CORTEX_EXPORT __attribute__((visibility("default")))
#define CORTEX_IMPORT /*!*/
#define CORTEX_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#define CORTEX_NO_RETURN __attribute__((no_return))
#define CORTEX_DEPRECATED __attribute__((__deprecated__))
#define CORTEX_PURE __attribute__((pure))
#define CORTEX_PACKED __attribute__((packed))
#define CORTEX_INIT __attribute__((constructor))
#define CORTEX_FINI __attribute__((destructor))
#if !defined(likely)
#define likely(x) __builtin_expect((x), 1)
#endif
#if !defined(unlikely)
#define unlikely(x) __builtin_expect((x), 0)
#endif
#elif defined(__MINGW32__)
#define CORTEX_NO_EXPORT /*!*/
#define CORTEX_EXPORT __declspec(export)
#define CORTEX_IMPORT __declspec(import)
#define CORTEX_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#define CORTEX_NO_RETURN __attribute__((no_return))
#define CORTEX_DEPRECATED __attribute__((__deprecated__))
#define CORTEX_PURE __attribute__((pure))
#define CORTEX_PACKED __attribute__((packed))
#define CORTEX_INIT /*!*/
#define CORTEX_FINI /*!*/
#if !defined(likely)
#define likely(x) (x)
#endif
#if !defined(unlikely)
#define unlikely(x) (x)
#endif
#elif defined(__MSVC__)
#define CORTEX_NO_EXPORT /*!*/
#define CORTEX_EXPORT __declspec(export)
#define CORTEX_IMPORT __declspec(import)
#define CORTEX_WARN_UNUSED_RESULT /*!*/
#define CORTEX_NO_RETURN          /*!*/
#define CORTEX_DEPRECATED         /*!*/
#define CORTEX_PURE               /*!*/
#define CORTEX_PACKED __packed    /* ? */
#define CORTEX_INIT               /*!*/
#define CORTEX_FINI               /*!*/
#if !defined(likely)
#define likely(x) (x)
#endif
#if !defined(unlikely)
#define unlikely(x) (x)
#endif
#else
#warning Unknown platform
#define CORTEX_NO_EXPORT          /*!*/
#define CORTEX_EXPORT             /*!*/
#define CORTEX_IMPORT             /*!*/
#define CORTEX_WARN_UNUSED_RESULT /*!*/
#define CORTEX_NO_RETURN          /*!*/
#define CORTEX_DEPRECATED         /*!*/
#define CORTEX_PURE               /*!*/
#define CORTEX_PACKED             /*!*/
#define CORTEX_INIT               /*!*/
#define CORTEX_FINI               /*!*/
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

#if defined(CORTEX_ENABLE_NOEXCEPT)
#define CORTEX_NOEXCEPT noexcept
#else
#define CORTEX_NOEXCEPT /*CORTEX_NOEXCEPT*/
#endif

#endif
