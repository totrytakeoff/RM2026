#!/usr/bin/env python3
"""
原生CMSIS-DAP固件烧录脚本
不使用OpenOCD，直接通过CMSIS-DAP协议与STM32F407通信
"""

import sys
import struct
import time
from typing import Optional

try:
    import usb.core
    import usb.util
except ImportError:
    print("错误: 需要安装pyusb库")
    print("请运行: pip install pyusb")
    sys.exit(1)

# CMSIS-DAP命令定义
CMD_INFO = 0x00
CMD_DAP_LED = 0x01
CMD_DAP_CONNECT = 0x02
CMD_DAP_DISCONNECT = 0x03
CMD_DAP_WRITE_ABORT = 0x08
CMD_DAP_DELAY = 0x09
CMD_DAP_RESET_TARGET = 0x0A
CMD_DAP_SWJ_PINS = 0x12
CMD_DAP_SWJ_CLOCK = 0x13
CMD_DAP_SWJ_SEQUENCE = 0x14
CMD_DAP_SWD_CONFIGURE = 0x15
CMD_DAP_TRANSFER_CONFIGURE = 0x16
CMD_DAP_TRANSFER = 0x17
CMD_DAP_TRANSFER_BLOCK = 0x18
CMD_DAP_TRANSFER_ABORT = 0x19

# DAP Transfer请求掩码
DAP_TRANSFER_RnW = (1 << 0)
DAP_TRANSFER_A32 = (1 << 1)
DAP_TRANSFER_MATCH_VALUE = (1 << 4)
DAP_TRANSFER_MATCH_MASK = (1 << 5)

# SWD寄存器定义
DP_IDCODE = 0x00
DP_ABORT = 0x00
DP_CTRL_STAT = 0x04
DP_SELECT = 0x08
DP_RDBUFF = 0x0C

# ARM Cortex-M4寄存器定义 (AP 0)
CSW = 0x00
TAR = 0x04
DRW = 0x0C

# CSW寄存器位定义
CSW_SIZE_8BIT = 0x00
CSW_SIZE_16BIT = 0x01
CSW_SIZE_32BIT = 0x02
CSW_SIZE_MASK = 0x07
CSW_ADDRINC_OFF = 0x00
CSW_ADDRINC_SINGLE = 0x10
CSW_DEVICE_EN = 0x01
CSW_SGEN_EN = 0x40
CSW_DBG_EN = 0x80

# STM32F407 Flash寄存器
FLASH_KEYR = 0x40023C04
FLASH_SR = 0x40023C0C
FLASH_CR = 0x40023C10
FLASH_AR = 0x40023C14

# Flash密钥
FLASH_KEY1 = 0x45670123
FLASH_KEY2 = 0xCDEF89AB

# STM32F407特定定义
FLASH_BASE = 0x08000000
FLASH_SECTOR_SIZE = 0x10000  # 64KB sectors
STM32F4_IDCODE = 0x2BA01477

class CMSISDAPFlasher:
    def __init__(self):
        self.device = None
        self.ep_out = None
        self.ep_in = None

    def find_device(self) -> bool:
        """查找CMSIS-DAP设备"""
        # 查找VID:PID = faed:4870 (Horco CMSIS-DAP)
        dev = usb.core.find(idVendor=0xfaed, idProduct=0x4870)
        if dev is None:
            print("错误: 未找到CMSIS-DAP设备 (faed:4870)")
            return False

        self.device = dev
        print(f"找到CMSIS-DAP设备: {self.device}")
        return True

    def setup_device(self) -> bool:
        """设置USB设备"""
        try:
            # 释放内核驱动
            for cfg in self.device:
                for intf in cfg:
                    if self.device.is_kernel_driver_active(intf.bInterfaceNumber):
                        self.device.detach_kernel_driver(intf.bInterfaceNumber)

            # 设置配置
            self.device.set_configuration()

            # 查找端点
            cfg = self.device.get_active_configuration()
            intf = cfg[(0, 0)]

            self.ep_out = usb.util.find_descriptor(
                intf,
                custom_match=lambda e: usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_OUT
            )

            self.ep_in = usb.util.find_descriptor(
                intf,
                custom_match=lambda e: usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_IN
            )

            if self.ep_out is None or self.ep_in is None:
                print("错误: 无法找到USB端点")
                return False

            print(f"USB端点配置完成: OUT={self.ep_out}, IN={self.ep_in}")
            return True

        except Exception as e:
            print(f"USB设备设置失败: {e}")
            return False

    def dap_command(self, cmd: int, data: bytes = b'') -> bytes:
        """发送CMSIS-DAP命令"""
        packet = bytes([cmd]) + data
        bytes_written = self.ep_out.write(packet)

        # 读取响应
        response = self.ep_in.read(64)
        return bytes(response)

    def dap_connect(self) -> bool:
        """连接到目标设备"""
        print("连接到目标设备...")
        response = self.dap_command(CMD_DAP_CONNECT, b'\x01')  # SWD模式

        if len(response) < 2 or response[0] != CMD_DAP_CONNECT:
            print("连接命令失败")
            return False

        status = response[1]
        if status == 0:
            print("连接失败")
            return False
        elif status == 1:
            print("已连接 (SWD模式)")
            return True

        return False

    def dap_configure(self) -> bool:
        """配置CMSIS-DAP"""
        print("配置CMSIS-DAP...")

        # 设置时钟频率 (2MHz)
        response = self.dap_command(CMD_DAP_SWJ_CLOCK, struct.pack('<I', 2000000))
        if response[0] != CMD_DAP_SWJ_CLOCK or response[1] != 0:
            print("时钟设置失败")
            return False

        # 配置传输
        transfer_config = struct.pack('<IHB', 64, 8, 0)
        response = self.dap_command(CMD_DAP_TRANSFER_CONFIGURE, transfer_config)
        if response[0] != CMD_DAP_TRANSFER_CONFIGURE or response[1] != 0:
            print("传输配置失败")
            return False

        # 配置SWD
        swd_config = b'\x00'  # 1个数据位
        response = self.dap_command(CMD_DAP_SWD_CONFIGURE, swd_config)
        if response[0] != CMD_DAP_SWD_CONFIGURE or response[1] != 0:
            print("SWD配置失败")
            return False

        print("CMSIS-DAP配置完成")
        return True

    def dap_transfer(self, requests: bytes) -> tuple:
        """执行DAP传输"""
        response = self.dap_command(CMD_DAP_TRANSFER, requests)
        if len(response) < 2:
            return False, b''

        count = response[1]
        data = response[2:2+count*4]
        return response[1] == len(requests), data

    def read_memory_word(self, address: int) -> Optional[int]:
        """读取32位内存"""
        # 构建传输请求：读取DP/AP寄存器
        requests = []

        # 选择AP 0 (地址0xC, APANK=0, Bank=0)
        requests.append(0x0C | (0 << 2) | DAP_TRANSFER_RnW)  # DP SELECT
        requests.extend(struct.pack('<I', 0x00000000))

        # 读取AP寄存器 CSW
        requests.append(0x00 | (1 << 2) | DAP_TRANSFER_RnW)  # AP CSW (地址0)

        # 设置TAR地址
        requests.append(0x04 | (1 << 2))  # AP TAR (地址4)
        requests.extend(struct.pack('<I', address))

        # 读取数据
        requests.append(0x0C | (1 << 2) | DAP_TRANSFER_RnW)  # AP DRW (地址0xC)

        # 执行传输
        success, data = self.dap_transfer(bytes(requests))
        if success and len(data) >= 4:
            return struct.unpack('<I', data[-4:])[0]

        return None

    def write_memory_word(self, address: int, value: int) -> bool:
        """写入32位内存"""
        # 构建传输请求
        requests = []

        # 选择AP 0
        requests.append(0x0C | (0 << 2))  # DP SELECT
        requests.extend(struct.pack('<I', 0x00000000))

        # 写入CSW (32位写入，地址递增关闭)
        csw_value = CSW_SIZE_32BIT | CSW_DEVICE_EN | CSW_DBG_EN
        requests.append(0x00 | (1 << 2))  # AP CSW
        requests.extend(struct.pack('<I', csw_value))

        # 设置TAR地址
        requests.append(0x04 | (1 << 2))  # AP TAR
        requests.extend(struct.pack('<I', address))

        # 写入数据
        requests.append(0x0C | (1 << 2))  # AP DRW
        requests.extend(struct.pack('<I', value))

        # 执行传输
        success, _ = self.dap_transfer(bytes(requests))
        return success

    def unlock_flash(self) -> bool:
        """解锁Flash"""
        print("解锁Flash...")

        # 检查Flash锁定状态
        flash_cr = self.read_memory_word(FLASH_CR)
        if flash_cr is None:
            return False

        if flash_cr & (1 << 31):  # LOCK位
            print("Flash已锁定，正在解锁...")

            # 写入解锁密钥
            if not self.write_memory_word(FLASH_KEYR, FLASH_KEY1):
                return False
            if not self.write_memory_word(FLASH_KEYR, FLASH_KEY2):
                return False

            # 再次检查锁定状态
            flash_cr = self.read_memory_word(FLASH_CR)
            if flash_cr is None or (flash_cr & (1 << 31)):
                print("Flash解锁失败")
                return False

        print("Flash解锁成功")
        return True

    def erase_flash(self, address: int, size: int) -> bool:
        """擦除Flash扇区"""
        print(f"擦除Flash: 0x{address:08X} - {size} 字节")

        if not self.unlock_flash():
            return False

        # 扇区擦除
        flash_cr = self.read_memory_word(FLASH_CR)
        if flash_cr is None:
            return False

        # 清除所有控制位
        flash_cr &= ~0xFFFC0000

        # 设置SER位 (扇区擦除)
        flash_cr |= (1 << 1)
        self.write_memory_word(FLASH_CR, flash_cr)

        # 设置要擦除的扇区
        sector_num = (address - FLASH_BASE) // FLASH_SECTOR_SIZE
        if sector_num > 11:
            print("扇区号超出范围")
            return False

        flash_cr |= (sector_num << 3)
        self.write_memory_word(FLASH_CR, flash_cr)

        # 开始擦除
        flash_cr |= (1 << 16)  # STRT位
        self.write_memory_word(FLASH_CR, flash_cr)

        # 等待擦除完成
        for _ in range(100):  # 最多等待1秒
            flash_sr = self.read_memory_word(FLASH_SR)
            if flash_sr is None:
                return False

            if flash_sr & (1 << 0):  # BSY位
                time.sleep(0.01)
                continue

            if flash_sr & (1 << 5):  # 读取错误
                print("擦除过程中发生读取错误")
                return False

            if flash_sr & (1 << 4):  # 编程错误
                print("擦除过程中发生编程错误")
                return False

            break

        print(f"扇区 {sector_num} 擦除完成")
        return True

    def program_flash(self, address: int, data: bytes) -> bool:
        """编程Flash"""
        print(f"编程Flash: 0x{address:08X} - {len(data)} 字节")

        if not self.unlock_flash():
            return False

        # 设置PG位 (编程)
        flash_cr = self.read_memory_word(FLASH_CR)
        if flash_cr is None:
            return False

        flash_cr &= ~0xFFFC0000
        flash_cr |= (1 << 0)  # PG位
        self.write_memory_word(FLASH_CR, flash_cr)

        # 按字写入数据
        for i in range(0, len(data), 4):
            if i + 4 > len(data):
                # 最后一个字，补充0xFF
                word_data = data[i:] + b'\xFF' * (4 - len(data[i:]))
            else:
                word_data = data[i:i+4]

            word_value = struct.unpack('<I', word_data)[0]

            if not self.write_memory_word(address + i, word_value):
                print(f"写入失败: 0x{address + i:08X}")
                return False

            # 等待写入完成
            for _ in range(10):
                flash_sr = self.read_memory_word(FLASH_SR)
                if flash_sr is None:
                    return False

                if flash_sr & (1 << 0):  # BSY位
                    time.sleep(0.001)
                    continue

                if flash_sr & (1 << 4):  # 编程错误
                    print(f"编程错误: 0x{address + i:08X}")
                    return False

                break

        print("Flash编程完成")
        return True

    def verify_flash(self, address: int, data: bytes) -> bool:
        """验证Flash内容"""
        print(f"验证Flash: 0x{address:08X} - {len(data)} 字节")

        for i in range(0, len(data), 4):
            if i + 4 > len(data):
                break

            expected = struct.unpack('<I', data[i:i+4])[0]
            actual = self.read_memory_word(address + i)

            if actual is None:
                print(f"验证读取失败: 0x{address + i:08X}")
                return False

            if actual != expected:
                print(f"验证失败: 0x{address + i:08X}, 期望: 0x{expected:08X}, 实际: 0x{actual:08X}")
                return False

        print("Flash验证成功")
        return True

    def reset_target(self) -> bool:
        """复位目标设备"""
        print("复位目标设备...")
        response = self.dap_command(CMD_DAP_RESET_TARGET)
        return len(response) > 1 and response[1] == 0

    def disconnect(self) -> None:
        """断开连接"""
        try:
            self.dap_command(CMD_DAP_DISCONNECT)
            if self.device:
                usb.util.dispose_resources(self.device)
        except:
            pass

    def flash_firmware(self, firmware_path: str) -> bool:
        """烧录固件"""
        print(f"开始烧录固件: {firmware_path}")

        # 读取固件文件
        try:
            with open(firmware_path, 'rb') as f:
                firmware_data = f.read()
        except Exception as e:
            print(f"读取固件文件失败: {e}")
            return False

        print(f"固件大小: {len(firmware_data)} 字节")

        # 对齐到4字节
        if len(firmware_data) % 4 != 0:
            firmware_data += b'\xFF' * (4 - len(firmware_data) % 4)

        # 查找并连接CMSIS-DAP设备
        if not self.find_device():
            return False

        if not self.setup_device():
            return False

        try:
            if not self.dap_connect():
                return False

            if not self.dap_configure():
                return False

            # 验证目标设备
            idcode = self.read_memory_word(DP_IDCODE)
            if idcode is None:
                print("无法读取IDCODE")
                return False

            print(f"目标IDCODE: 0x{idcode:08X}")
            if (idcode & 0xFFF) != 0xBA4:  # Cortex-M4标识
                print("警告: 目标设备可能不是Cortex-M4")

            # 擦除Flash
            if not self.erase_flash(FLASH_BASE, len(firmware_data)):
                return False

            # 编程Flash
            if not self.program_flash(FLASH_BASE, firmware_data):
                return False

            # 验证Flash
            if not self.verify_flash(FLASH_BASE, firmware_data):
                return False

            # 复位目标设备
            if not self.reset_target():
                print("复位失败，但烧录可能成功")

            print("固件烧录成功完成!")
            return True

        except Exception as e:
            print(f"烧录过程中发生错误: {e}")
            return False

        finally:
            self.disconnect()

def main():
    if len(sys.argv) != 2:
        print("用法: python3 cmsis_dap_upload.py <firmware.bin>")
        sys.exit(1)

    firmware_path = sys.argv[1]

    flasher = CMSISDAPFlasher()
    success = flasher.flash_firmware(firmware_path)

    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()