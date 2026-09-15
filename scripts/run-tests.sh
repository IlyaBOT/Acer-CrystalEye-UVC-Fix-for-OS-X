#!/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP_BASE="${TMPDIR:-/tmp}"
TEST_DIR="$TMP_BASE/crystaleye-tests-$$"
trap 'rm -rf "$TEST_DIR"' EXIT HUP INT TERM
mkdir -p "$TEST_DIR"

cc -std=c99 -Wall -Wextra -Werror -I"$ROOT/Sources/Common" "$ROOT/Sources/Common/UVCStreamingControl.c" "$ROOT/Tests/test_uvc_streaming_control.c" -o "$TEST_DIR/test-uvc"
"$TEST_DIR/test-uvc"

