#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd "$(dirname "$0")/.." && pwd)"
pio_cmd="${PIO_CMD:-$HOME/.platformio/penv/bin/pio}"
python_cmd="${PIO_PYTHON:-$HOME/.platformio/penv/bin/python}"
esptool_py="${ESPTOOL_PY:-$HOME/.platformio/packages/tool-esptoolpy/esptool.py}"
framework_dir="${ARDUINO_ESP32_DIR:-$HOME/.platformio/packages/framework-arduinoespressif32}"
build_dir="$project_dir/.pio/build/esp32dev"
release_dir="$project_dir/release"

"$pio_cmd" run --project-dir "$project_dir" -e esp32dev
mkdir -p "$release_dir"
cp "$build_dir/firmware.bin" "$release_dir/astro-orbit-firmware.bin"

"$python_cmd" "$esptool_py" --chip esp32 merge_bin \
  -o "$release_dir/astro-orbit-factory.bin" \
  --flash_mode dio --flash_freq 40m --flash_size 4MB \
  0x1000 "$build_dir/bootloader.bin" \
  0x8000 "$build_dir/partitions.bin" \
  0xe000 "$framework_dir/tools/partitions/boot_app0.bin" \
  0x10000 "$build_dir/firmware.bin"

cd "$release_dir"
if command -v shasum >/dev/null 2>&1; then
  shasum -a 256 astro-orbit-factory.bin astro-orbit-firmware.bin > SHA256SUMS.txt
else
  sha256sum astro-orbit-factory.bin astro-orbit-firmware.bin > SHA256SUMS.txt
fi
printf 'Release files ready in %s\n' "$release_dir"
