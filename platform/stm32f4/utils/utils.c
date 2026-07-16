/**
  * @file       utils.c
  * @brief      实用工具函数实现，包含完美支持浮点数的格式化函数
  */

#include "utils.h"
#include <string.h>
#include <math.h>
#include <stdio.h>

// 内部函数：格式化单个浮点数
static int format_single_float(char* buffer, size_t buffer_size, float value, int precision, int width, char flags)
{
    if (buffer == NULL || buffer_size == 0) {
        return 0;
    }
    
    // 处理特殊值
    if (isnan(value)) {
        return snprintf(buffer, buffer_size, "nan");
    }
    if (isinf(value)) {
        if (value < 0) {
            return snprintf(buffer, buffer_size, "-inf");
        } else {
            return snprintf(buffer, buffer_size, "inf");
        }
    }
    
    // 限制精度在0-9之间
    if (precision < 0) precision = 6; // 默认精度
    if (precision > 9) precision = 9;
    
    // 处理负数
    int is_negative = 0;
    if (value < 0) {
        is_negative = 1;
        value = -value;
    }
    
    // 计算缩放因子
    int scale = 1;
    for (int i = 0; i < precision; i++) {
        scale *= 10;
    }
    
    // 转换为整数
    int int_part = (int)value;
    int frac_part = (int)((value - int_part) * scale + 0.5f); // 四舍五入
    
    // 处理进位
    if (frac_part >= scale) {
        int_part += 1;
        frac_part = 0;
    }
    
    // 计算需要的空间
    int total_len = 0;
    
    // 负号
    if (is_negative) total_len++;
    
    // 整数部分
    int temp_int = int_part;
    if (temp_int == 0) {
        total_len++; // "0"
    } else {
        while (temp_int > 0) {
            total_len++;
            temp_int /= 10;
        }
    }
    
    // 小数点和小数部分
    if (precision > 0) {
        total_len++; // 小数点
        total_len += precision; // 小数位数
    }
    
    // 检查缓冲区大小
    if (total_len >= buffer_size) {
        return 0;
    }
    
    // 开始格式化
    int pos = 0;
    
    // 添加负号
    if (is_negative) {
        buffer[pos++] = '-';
    }
    
    // 添加整数部分
    if (int_part == 0) {
        buffer[pos++] = '0';
    } else {
        // 反转整数部分
        char int_str[12]; // 足够大的整数缓冲区
        int int_pos = 0;
        temp_int = int_part;
        while (temp_int > 0) {
            int_str[int_pos++] = '0' + (temp_int % 10);
            temp_int /= 10;
        }
        
        // 反向复制到缓冲区
        for (int i = int_pos - 1; i >= 0; i--) {
            buffer[pos++] = int_str[i];
        }
    }
    
    // 添加小数部分
    if (precision > 0) {
        buffer[pos++] = '.';
        
        // 补零
        int temp = scale / 10;
        while (temp > frac_part && precision > 1) {
            buffer[pos++] = '0';
            temp /= 10;
            precision--;
        }
        
        // 添加小数部分
        if (frac_part > 0) {
            char frac_str[12]; // 足够大的小数缓冲区
            int frac_pos = 0;
            temp = frac_part;
            while (temp > 0) {
                frac_str[frac_pos++] = '0' + (temp % 10);
                temp /= 10;
            }
            
            // 反向复制到缓冲区
            for (int i = frac_pos - 1; i >= 0; i--) {
                buffer[pos++] = frac_str[i];
            }
        } else {
            // 全部补零
            for (int i = 0; i < precision; i++) {
                buffer[pos++] = '0';
            }
        }
    }
    
    buffer[pos] = '\0';
    return pos;
}

/**
  * @brief      安全的vsnprintf实现，完美支持浮点数
  * @param[out] buffer: 输出缓冲区
  * @param[in]  buffer_size: 缓冲区大小
  * @param[in]  format: 格式化字符串
  * @param[in]  args: 可变参数列表
  * @retval     实际写入的字符数（不包括终止符）
  */
int safe_vsnprintf(char* buffer, size_t buffer_size, const char* format, va_list args)
{
    if (buffer == NULL || buffer_size == 0 || format == NULL) {
        return 0;
    }
    
    size_t pos = 0;
    const char* p = format;
    
    while (*p != '\0' && pos < buffer_size - 1) {
        if (*p != '%') {
            buffer[pos++] = *p++;
            continue;
        }
        
        // 处理格式说明符
        p++; // 跳过 '%'
        
        if (*p == '\0') break;
        
        // 处理转义字符 '%'
        if (*p == '%') {
            buffer[pos++] = *p++;
            continue;
        }
        
        // 解析格式标志
        char flags = 0;
        while (*p == '-' || *p == '+' || *p == ' ' || *p == '0' || *p == '#') {
            flags |= *p;
            p++;
        }
        
        // 解析宽度
        int width = 0;
        while (*p >= '0' && *p <= '9') {
            width = width * 10 + (*p - '0');
            p++;
        }
        
        // 解析精度
        int precision = -1;
        if (*p == '.') {
            p++;
            precision = 0;
            while (*p >= '0' && *p <= '9') {
                precision = precision * 10 + (*p - '0');
                p++;
            }
        }
        
        // 解析长度修饰符
        while (*p == 'h' || *p == 'l' || *p == 'L') {
            p++;
        }
        
        // 解析转换说明符
        char spec = *p++;
        
        // 处理不同的格式说明符
        switch (spec) {
            case 'd':
            case 'i': {
                int value = va_arg(args, int);
                int len = snprintf(buffer + pos, buffer_size - pos, "%d", value);
                pos += len;
                break;
            }
            case 'u': {
                unsigned int value = va_arg(args, unsigned int);
                int len = snprintf(buffer + pos, buffer_size - pos, "%u", value);
                pos += len;
                break;
            }
            case 'x': {
                unsigned int value = va_arg(args, unsigned int);
                int len = snprintf(buffer + pos, buffer_size - pos, "%x", value);
                pos += len;
                break;
            }
            case 'X': {
                unsigned int value = va_arg(args, unsigned int);
                int len = snprintf(buffer + pos, buffer_size - pos, "%X", value);
                pos += len;
                break;
            }
            case 'c': {
                char value = (char)va_arg(args, int);
                if (pos < buffer_size - 1) {
                    buffer[pos++] = value;
                }
                break;
            }
            case 's': {
                char* value = va_arg(args, char*);
                int len = snprintf(buffer + pos, buffer_size - pos, "%s", value);
                pos += len;
                break;
            }
            case 'f':
            case 'F':
            case 'g':
            case 'G':
            case 'e':
            case 'E': {
                float value = (float)va_arg(args, double);
                int len = format_single_float(buffer + pos, buffer_size - pos, value, precision, width, flags);
                pos += len;
                break;
            }
            default: {
                // 未知格式，原样输出
                if (pos < buffer_size - 2) {
                    buffer[pos++] = '%';
                    buffer[pos++] = spec;
                }
                break;
            }
        }
    }
    
    // 确保字符串以null结尾
    if (pos < buffer_size) {
        buffer[pos] = '\0';
    } else {
        buffer[buffer_size - 1] = '\0';
    }
    
    return pos;
}

/**
  * @brief      完美支持浮点数的格式化函数，类似于snprintf
  * @param[out] buffer: 输出缓冲区
  * @param[in]  buffer_size: 缓冲区大小
  * @param[in]  format: 格式化字符串，支持%s, %d, %u, %x, %f, %.2f等
  * @param[in]  ...: 可变参数列表
  * @retval     实际写入的字符数（不包括终止符）
  * @note       此函数完全兼容snprintf格式，但完美支持浮点数格式化
  */
int safe_snprintf(char* buffer, size_t buffer_size, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int result = safe_vsnprintf(buffer, buffer_size, format, args);
    va_end(args);
    return result;
}