#!/bin/bash
# DFU upload script for STM32F407 (0483:df11)
# This bypasses PlatformIO's DFU suffix mechanism which uses wrong VID:PID

BUILD_DIR="${1:-.pio/build/dfu}"
FW_ELF="$BUILD_DIR/firmware.elf"
FW_BIN="/tmp/firmware_dfu.bin"

# Check if ELF exists
if [ ! -f "$FW_ELF" ]; then
    echo "ERROR: firmware.elf not found at $FW_ELF"
    exit 1
fi

echo "[DFU] Extracting binary from ELF..."
/home/myself/.platformio/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-objcopy -O binary "$FW_ELF" "$FW_BIN"

if [ ! -f "$FW_BIN" ]; then
    echo "ERROR: Failed to create firmware binary"
    exit 1
fi

SIZE=$(stat -f%z "$FW_BIN" 2>/dev/null || stat -c%s "$FW_BIN" 2>/dev/null)
echo "[DFU] Binary ready: $FW_BIN ($SIZE bytes)"

echo "[DFU] Uploading to STM32F407 (0483:df11)..."
/home/myself/.platformio/packages/tool-dfuutil/bin/dfu-util -d 0483:df11 -a 0 -s 0x08000000:leave -D "$FW_BIN"

RESULT=$?
rm -f "$FW_BIN"

if [ $RESULT -eq 0 ]; then
    echo "[DFU] ✓ Upload successful!"
else
    echo "[DFU] ✗ Upload failed (exit code: $RESULT)"
fi

exit $RESULT
