libstx
======

The missing extended standard library for C++ ;)

While we do extensively use this code in production it is currently lacking any documentation.

Contributions always welcome.

- github: https://github.com/fnordcorp/libstx

## Including libstx via CMake

All you need to do is to put this into your CMakeLists.txt:

    include(my/path/to/libstx/cmake/libstx.cmake)


## Build-time dependencies

- gcc >= 4.8.0 (for building only, clang >= 3.4 with libc++)
- cmake (for building only)

## Runtime dependencies

- (optional) OpenSSL (libssl-dev)

### Install From Source: Ubuntu

```
apt-get install git clang++ cmake pkg-config libssl-dev libbz2-dev
git clone git@github.com:fnordcorp/libstx.git
cd libstx
make debug
```

## Included 3rdparty Software

- google's protocol buffer library
- libsimdcomp

## Contributors

- Laura Schlimmer (https://github.com/lauraschlimmer)
- Christian Parpart (https://github.com/christianparpart)
- Paul Asmuth (https://github.com/paulasmuth)

LICENSE
-------

```
Copyright (c) The libstx authors

libstx is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

libstx is distributed in the hope that it will be useful,but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
libstx. If not, see <http://www.gnu.org/licenses/>.
