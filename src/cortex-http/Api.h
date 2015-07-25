// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#ifndef cortex_http_api_hpp
#define cortex_http_api_hpp (1)

#include <cortex-base/Defines.h>

// libcortex-http exports
#if defined(BUILD_CORTEX_HTTP)
#define CORTEX_HTTP_API CORTEX_EXPORT
#else
#define CORTEX_HTTP_API CORTEX_IMPORT
#endif

#endif
