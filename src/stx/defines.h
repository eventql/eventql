// This file is part of the "libstx" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libstx is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#ifndef libstx_defines_h
#define libstx_defines_h (1)

#include <stx/sysconfig.h>
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
