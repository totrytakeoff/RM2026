#!/bin/bash
# STM32 调试脚本（OpenOCD），启动OpenOCD服务器用于GDB连接
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEFAULT_CFG="${ROOT_DIR}/config/openocd/openocd_dap.cfg"
DEFAULT_PORT="3333"

usage() {
    cat <<EOF
用法: $(basename "$0") [--cfg <openocd.cfg>] [--port <port>] [--openocd <path>]
示例:
  $(basename "$0")                      # 使用默认配置启动OpenOCD服务器
  $(basename "$0") --cfg config/openocd/openocd_jlink.cfg --port 4444
EOF
}

CFG="$DEFAULT_CFG"
PORT="$DEFAULT_PORT"
OPENOCD_BIN=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --cfg|--config) CFG="$2"; shift 2;;
        --port) PORT="$2"; shift 2;;
        --openocd) OPENOCD_BIN="$2"; shift 2;;
        -h|--help) usage; exit 0;;
        *) echo "未知参数: $1"; usage; exit 1;;
    esac
done

if [[ "$CFG" != /* && "$CFG" != "~/"* ]]; then
    CFG="${ROOT_DIR}/${CFG}"
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

echo "OpenOCD: $OPENOCD_BIN"
echo "Config : $CFG"
echo "Port   : $PORT"
echo "启动OpenOCD服务器，按Ctrl+C停止..."

# 启动OpenOCD服务器
"$OPENOCD_BIN" -f "$CFG" -c "init; halt; reset init"