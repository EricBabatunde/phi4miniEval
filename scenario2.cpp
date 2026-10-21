#include <array>
#include <cstdint>
#include <iostream>

template<size_t N>
class CircularBuffer {
public:
    CircularBuffer() : head(0), tail(0), count(0) {
        buffer.resize(N);
    }

    bool push(uint8_t val) {
        if (count == N) {
            // Buffer is full
            return false;
        }
        buffer[head] = val;
        head = (head + 1) % N;
        ++count;
        return true;
    }

    bool pop(uint8_t *val) {
        if (count == 0) {
            // Buffer is empty
            return false;
        }
        *val = buffer[tail];
        tail = (tail + 1) % N;
        --count;
        return true;
    }

    bool isFull() const {
        return count == N;
    }

    bool isEmpty() const {
        return count == 0;
    }

private:
    std::array<uint8_t, N> buffer;
    size_t head;
    size_t tail;
    size_t count;
};

int main() {
    CircularBuffer<16> cb;

    // Test pushing data into the buffer
    for (uint8_t i = 0; i < 16; ++i) {
        if (cb.push(i)) {
            std::cout << "Pushed: " << static_cast<int>(i) << std::endl;
        } else {
            std::cout << "Buffer is full. Cannot push: " << static_cast<int>(i) << std::endl;
        }
    }

    // Test popping data from the buffer
    for (uint8_t i = 0; i < 16; ++i) {
        uint8_t val;
        if (cb.pop(&val)) {
            std::cout << "Popped: " << static_cast<int>(val) << " at head: " << static_cast<int>(cb.head) << std::endl;
        } else {
            std::cout << "Buffer is empty. Cannot pop." << std::endl;
        }
    }

    // Test pushing again after emptying the buffer
    if (cb.push(16)) {
        std::cout << "Pushed: 16" << std::endl;
    }

    // Test popping again after pushing
    if (cb.pop(&val)) {
        std::cout << "Popped: " << static_cast<int>(val) << " at head: " << static_cast<int>(cb.head) << std::endl;
    }

    return 0;
}
