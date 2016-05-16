#!/bin/bash
set -e

BUILD_ID=$1
if [[ -z "$BUILD_ID" ]]; then
  echo "usage: $0 <BUILD_ID>";
  exit 1
fi

BUILD_TYPE=${BUILD_TYPE:-release}
BUILD_DIR=${BUILD_DIR:-./build}
BUILD_ARCH=${BUILD_ARCH:-$(uname -m)}
BUILD_OS=${BUILD_OS:-$(uname | tr '[:upper:]' '[:lower:]')}
BUILD_NCPUS=${BUILD_NCPUS:-$[ $(getconf _NPROCESSORS_ONLN) / 2 ]}
BUILD_DOCUMENTATION=${BUILD_DOCUMENTATION:-true}
BUILD_ASSETS=${BUILD_ASSETS:-true}
BUILD_ARTIFACTS=${BUILD_ARTIFACTS:-true}
RUN_TESTS=${RUN_TESTS:-true}

if [[ "${BUILD_ARCH}" == "x86" ]]; then
  ARCHFLAGS="-m32 -march=i386";
fi

if [[ "${BUILD_ARCH}" == "x86_64" ]]; then
  ARCHFLAGS="-m64 -march=x86-64";
fi

if [[ -z "$MAKETOOL" ]]; then
  MAKETOOL="make"
  #if which ninja > /dev/null; then
  #  MAKETOOL="ninja"
  #fi
fi

echo    "======================  zScale Z1 ======================="
echo -e "                                                         "
echo -e "    Build-ID:      ${BUILD_ID}"
echo -e "    Build-Type:    ${BUILD_TYPE}"
echo -e "    Build-Target:  ${BUILD_OS}/${BUILD_ARCH}"
echo -e "    Maketool:      ${MAKETOOL}"
echo -e "                                                         "
echo    "========================================================="

TARGET_LBL="${BUILD_TYPE}_${BUILD_OS}_${BUILD_ARCH}"
TARGET_DIR="${BUILD_DIR}/${BUILD_ID}/${TARGET_LBL}"
ARTIFACTS_DIR="${BUILD_DIR}/${BUILD_ID}"
mkdir -p $TARGET_DIR
mkdir -p ${ARTIFACTS_DIR}
mkdir -p ${TARGET_DIR}/package
TARGET_DIR_REAL=$(cd ${TARGET_DIR} && pwd)
SOURCE_DIR_REAL=$(pwd)

# build c++ with make
if [[ $MAKETOOL == "make" ]]; then
  if [[ ! -e ${TARGET_DIR}/Makefile ]]; then
    CFLAGS="${ARCHFLAGS} ${CFLAGS}" \
    CXXFLAGS="${ARCHFLAGS} ${CXXFLAGS}" \
    ZBASE_BUILD_ID="${BUILD_ID}" cmake \
        -G "Unix Makefiles" \
        -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
        -B${TARGET_DIR_REAL} \
        -H${SOURCE_DIR_REAL}
  fi

  (cd ${TARGET_DIR} && make -j${BUILD_NCPUS}) || exit 1

# build c++ with ninja
elif [[ $MAKETOOL == "ninja" ]]; then
  if [[ ! -e ${TARGET_DIR}/build.ninja ]]; then
    CFLAGS="${ARCHFLAGS} ${CFLAGS}" \
    CXXFLAGS="${ARCHFLAGS} ${CXXFLAGS}" \
    ZBASE_BUILD_ID="${BUILD_ID}" cmake \
        -G "Ninja" \
        -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
        -B${TARGET_DIR_REAL} \
        -H${SOURCE_DIR_REAL}
  fi

  (cd ${TARGET_DIR} && ninja -j${BUILD_NCPUS}) || exit 1

else
  echo "error unknown build tool ${MAKETOOL}" >&2
  exit 1
fi

# test
if [[ $RUN_TESTS == "true" ]]; then
  find ${TARGET_DIR} -maxdepth 1 -name "test-*" -type f -exec ./{} \;
fi
