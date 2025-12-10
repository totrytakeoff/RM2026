#!/bin/bash
# STM32 烧录脚本（OpenOCD），从 build/ 下的 bin/hex/elf 生成路径并烧录
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
DEFAULT_FILE="Src/app"
DEFAULT_FORMAT="bin"
DEFAULT_ADDR="0x08000000"
DEFAULT_CFG="${ROOT_DIR}/config/openocd/openocd_dap.cfg"

usage() {
    cat <<EOF
用法: $(basename "$0") [--file <path-without-ext>] [--format bin|hex|elf] [--addr <0x...>] [--cfg <openocd.cfg>] [--openocd <path>] [--verify-only] [--dry-run]
示例:
  $(basename "$0")                      # 烧录 build/Src/app.bin 到 0x08000000
  $(basename "$0") --file tests/imu/test_imu --format hex
  $(basename "$0") --openocd /usr/bin/openocd --cfg config/openocd/openocd_dap.cfg
EOF
}

FILE="$DEFAULT_FILE"
FORMAT="$DEFAULT_FORMAT"
ADDR="$DEFAULT_ADDR"
CFG="$DEFAULT_CFG"
OPENOCD_BIN=""
VERIFY_ONLY=false
DRY_RUN=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        --file) FILE="$2"; shift 2;;
        --format) FORMAT="$2"; shift 2;;
        --addr|--address) ADDR="$2"; shift 2;;
        --cfg|--config) CFG="$2"; shift 2;;
        --openocd) OPENOCD_BIN="$2"; shift 2;;
        --verify-only) VERIFY_ONLY=true; shift;;
        --dry-run) DRY_RUN=true; shift;;
        -h|--help) usage; exit 0;;
        *) echo "未知参数: $1"; usage; exit 1;;
    esac
done

if [[ "$FORMAT" != "bin" && "$FORMAT" != "hex" && "$FORMAT" != "elf" ]]; then
    echo "格式必须为 bin|hex|elf"; exit 1
fi

if [[ "$CFG" != /* && "$CFG" != "~/"* ]]; then
    CFG="${ROOT_DIR}/${CFG}"
fi

resolve_file() {
    local path="$1"
    # 相对路径自动指向 build 下（不含扩展名）
    [[ "$path" == /* || "$path" == "~/"* ]] && echo "$path" && return
    echo "${BUILD_DIR}/${path}.${FORMAT}"
}

TARGET_FILE="$(resolve_file "$FILE")"
if [ ! -f "$TARGET_FILE" ]; then
    echo "未找到文件: $TARGET_FILE"
    exit 1
fi

if [ -z "$OPENOCD_BIN" ]; then
    OPENOCD_BIN="$(command -v openocd || true)"
    [ -z "$OPENOCD_BIN" ] && OPENOCD_BIN="${HOME}/.platformio/packages/tool-openocd/bin/openocd"
fi

if [ ! -x "$OPENOCD_BIN" ]; then
    echo "OpenOCD 不可用: $OPENOCD_BIN"
    exit 1
fi

if [ ! -f "$CFG" ]; then
    echo "OpenOCD 配置不存在: $CFG"
    exit 1
fi

CMD=(
    "$OPENOCD_BIN" -f "$CFG"
    -c "init; halt;"
)
if $VERIFY_ONLY; then
    CMD+=(-c "verify_image $TARGET_FILE $ADDR; reset; shutdown")
else
    CMD+=(-c "flash write_image erase $TARGET_FILE $ADDR; verify_image $TARGET_FILE $ADDR; reset; shutdown")
fi

echo "OpenOCD: $OPENOCD_BIN"
echo "Config : $CFG"
echo "File   : $TARGET_FILE"
echo "Addr   : $ADDR"
if $DRY_RUN; then
    printf 'Command: %q ' "${CMD[@]}"; echo
    exit 0
fi

"${CMD[@]}"
