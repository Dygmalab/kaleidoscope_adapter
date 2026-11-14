#!/usr/bin/env bash
# Host-side test of plugins/GraveEscapeDygma.cpp.
#
# Compiles the real plugin against the small stand-ins in stub/ (which shadow
# the Kaleidoscope headers it uses) plus the real Kaleidoscope-Ranges.h, and
# replays keyboard scans in row order. Needs a host C++11 compiler and the
# kaleidoscope submodule checked out.
#
# Usage: tests/GraveEscapeDygma/run.sh
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/gesc.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT
# Copy the plugin so its quoted includes resolve to stub/, not plugins/.
cp "$ROOT/plugins/GraveEscapeDygma.h" "$ROOT/plugins/GraveEscapeDygma.cpp" "$WORK/"
c++ -std=c++11 -Wall -Wextra -Werror \
  -I"$HERE/stub" -I"$WORK" -I"$ROOT/kaleidoscope/src" \
  "$HERE/test.cpp" "$WORK/GraveEscapeDygma.cpp" -o "$WORK/t"
"$WORK/t"
