#!/bin/sh
set -eu

if [ "$(uname -s)" != Linux ]; then
  echo "This regression test runner requires Linux." >&2
  exit 1
fi

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
amtl=${AMTL:-"$root/public/amtl"}
hlsdk=${HLSDK:-"$root/../hlsdk"}
metamod=${METAMOD:-"$root/../metamod-hl1"}
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

g++ -std=c++11 -pthread \
  -I"$amtl" \
  -I"$root/modules/mysqlx/thread" \
  -I"$root/modules/mysqlx" \
  "$root/modules/mysqlx/thread/tests/ThreadWorkerTests.cpp" \
  "$root/modules/mysqlx/thread/ThreadWorker.cpp" \
  "$root/modules/mysqlx/thread/BaseWorker.cpp" \
  "$root/modules/mysqlx/thread/PosixThreads.cpp" \
  -o "$tmpdir/thread_worker_tests"
"$tmpdir/thread_worker_tests"

g++ -std=c++11 -pthread \
  -DLINUX -D_LINUX -DPOSIX -DPLATFORM_POSIX -DPAWN_CELL_SIZE=32 \
  -I"$root/amxmodx" \
  -I"$root/public" \
  -I"$root/public/sdk" \
  -I"$root/public/resdk" \
  -I"$root/public/memtools" \
  -I"$root/third_party" \
  -I"$root/third_party/hashing" \
  -I"$root/third_party/zlib" \
  -I"$root/third_party/utf8rewind" \
  -I"$amtl" \
  -I"$hlsdk/common" \
  -I"$hlsdk/engine" \
  -I"$hlsdk/dlls" \
  -I"$hlsdk/public" \
  -I"$metamod/metamod" \
  "$root/amxmodx/tests/CoreRegressionTests.cpp" \
  "$root/amxmodx/CDataPack.cpp" \
  -Wl,--wrap=realloc \
  -o "$tmpdir/core_regression_tests"
"$tmpdir/core_regression_tests"

echo "All regression tests passed."
