#!/usr/bin/env python3
"""
简化的CMSIS-DAP烧录脚本，使用PlatformIO自带的OpenOCD工具
避免额外依赖，同时保持原生CMSIS-DAP接口的优势
"""

import sys
import subprocess
import os

def find_platformio_openocd():
    """查找PlatformIO安装的OpenOCD"""
    possible_paths = [
        os.path.expanduser("~/.platformio/packages/tool-openocd/bin/openocd"),
        os.path.expanduser("~/.platformio/packages/tool-openocd/openocd"),
        "/usr/bin/openocd",
        "/usr/local/bin/openocd"
    ]

    for path in possible_paths:
        if os.path.exists(path):
            return path

    return None

def flash_firmware(firmware_path: str) -> bool:
    """使用CMSIS-DAP接口烧录固件"""

    print(f"开始烧录固件: {firmware_path}")

    # 检查固件文件
    if not os.path.exists(firmware_path):
        print(f"错误: 固件文件不存在: {firmware_path}")
        return False

    # 查找OpenOCD
    openocd_path = find_platformio_openocd()
    if openocd_path:
        print(f"使用OpenOCD: {openocd_path}")
    else:
        print("错误: 未找到OpenOCD")
        return False

    # 构建OpenOCD命令 (使用CMSIS-DAP接口)
    cmd = [
        openocd_path,
        "-f", "interface/cmsis-dap.cfg",
        "-f", "target/stm32f4x.cfg",
        "-c", f"program {firmware_path} 0x08000000 verify reset exit"
    ]

    print(f"执行命令: {' '.join(cmd)}")

    try:
        # 执行OpenOCD烧录
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)

        print("OpenOCD输出:")
        print(result.stdout)

        if result.stderr:
            print("错误输出:")
            print(result.stderr)

        if result.returncode == 0:
            print("固件烧录成功完成!")
            return True
        else:
            print(f"烧录失败，返回码: {result.returncode}")
            return False

    except subprocess.TimeoutExpired:
        print("烧录超时")
        return False
    except Exception as e:
        print(f"执行烧录时发生错误: {e}")
        return False

def main():
    if len(sys.argv) != 2:
        print("用法: python3 cmsis_dap_simple.py <firmware.bin>")
        sys.exit(1)

    firmware_path = sys.argv[1]

    success = flash_firmware(firmware_path)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()