#!/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SDK="${SDKROOT:-/Developer/SDKs/MacOSX10.6.sdk}"
CC_BIN="${CC:-/usr/bin/gcc-4.2}"
BUILD="$ROOT/build"

if [ ! -x "$CC_BIN" ]; then
  echo "[build] ERROR: GCC 4.2 not found at $CC_BIN" >&2
  exit 1
fi
if [ ! -d "$SDK" ]; then
  echo "[build] ERROR: Snow Leopard SDK not found at $SDK" >&2
  exit 1
fi

mkdir -p "$BUILD"
echo "[build] compiler: $CC_BIN"
echo "[build] SDK: $SDK"
echo "[build] crystaleye-uvc (i386)"
"$CC_BIN" -arch i386 -isysroot "$SDK" -mmacosx-version-min=10.6 -std=gnu99 -Wall -Wextra -I"$ROOT/Sources/Common" "$ROOT/Sources/Common/UVCStreamingControl.c" "$ROOT/Sources/CrystalEyeProbe/main.c" -framework CoreFoundation -framework IOKit -o "$BUILD/crystaleye-uvc"

echo "[build] crystaleye-primer (i386)"
"$CC_BIN" -arch i386 -isysroot "$SDK" -mmacosx-version-min=10.6 -Wall -Wextra "$ROOT/Sources/CrystalEyePrimer/main.m" -framework Foundation -framework QTKit -framework CoreVideo -framework IOKit -o "$BUILD/crystaleye-primer"

file "$BUILD/crystaleye-uvc" "$BUILD/crystaleye-primer"
echo "[build] done"

