#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
json_dir="$project_dir/.pio/libdeps/esp32dev/ArduinoJson/src"
if [ ! -f "$json_dir/ArduinoJson.h" ]; then
    echo 'Run pio run -e esp32dev first to install ArduinoJson.' >&2
    exit 1
fi
test_binary=$(mktemp "${TMPDIR:-/tmp}/morota-usb-test.XXXXXX")
trap 'rm -f "$test_binary"' EXIT HUP INT TERM
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror \
    -I"$project_dir/tests/host" -I"$project_dir/include" -I"$json_dir" \
    "$project_dir/src/rotator_transport.cpp" "$project_dir/tests/test_usb_transport.cpp" \
    -o "$test_binary"
"$test_binary"
