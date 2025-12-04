#pragma once
#include <stdint.h>
#include <string.h> // for memcpy if needed

// T: 数据类型 (比如 uint8_t, 或者自定义结构体)
// Capacity: 缓冲区大小 (编译期常量)
template <typename T, uint32_t Capacity>
class RingBuffer {
public:
    RingBuffer() : head_(0), tail_(0), is_full_(false) {}

    // 重置缓冲区
    void clear() {
        head_ = 0;
        tail_ = 0;
        is_full_ = false;
    }

    // 推入数据
    bool push(const T& item) {
        // 如果满了，根据策略，可以选择覆盖旧数据或者拒绝写入
        // 这里演示拒绝写入
        if (isFull()) {
            return false; 
        }

        buffer_[head_] = item;
        head_ = (head_ + 1) % Capacity;
        
        // 这里的满标志逻辑根据具体实现可能不同，这是最简单的一种
        if (head_ == tail_) {
            is_full_ = true;
        }
        return true;
    }

    // 弹出一个数据
    bool pop(T& item) {
        if (isEmpty()) {
            return false;
        }

        item = buffer_[tail_];
        tail_ = (tail_ + 1) % Capacity;
        is_full_ = false; // 只要取出了数据，肯定就不满了
        return true;
    }

    // 强制推入（如果满了，覆盖最老的数据）- 适合传感器最新数据
    void forcePush(const T& item) {
        buffer_[head_] = item;
        head_ = (head_ + 1) % Capacity;
        
        if (is_full_) {
            // 如果本来就满了，头追上了尾，尾也要向前移，丢弃最老数据
            tail_ = (tail_ + 1) % Capacity;
        } else if (head_ == tail_) {
            is_full_ = true;
        }
    }

    // 获取当前元素个数
    uint32_t count() const {
        if (is_full_) return Capacity;
        if (head_ >= tail_) return head_ - tail_;
        return Capacity + head_ - tail_;
    }

    bool isFull() const { return is_full_; }
    bool isEmpty() const { return (!is_full_ && (head_ == tail_)); }
    uint32_t capacity() const { return Capacity; }

private:
    T buffer_[Capacity]; // 【核心】编译器直接分配静态数组，不是指针！
    volatile uint32_t head_; // volatile 防止编译器过度优化（如果在中断用）
    volatile uint32_t tail_;
    volatile bool is_full_;
};