#pragma once
#include <stddef.h>
// Fixed memory, no blocking reads. An overlong frame is discarded in full.
class UsbLineBuffer {
public:
    static const size_t Capacity = 255;
    bool push(char c) {
        if(c == '\n') {
            bool ready = !overflow && used != 0;
            data[used] = 0;
            used = 0;
            overflow = false;
            return ready;
        }
        if(c == '\r') return false;
        if(overflow) return false;
        if(used == Capacity) { overflow = true; return false; }
        data[used++] = c;
        return false;
    }
    const char* line() const { return data; }
    bool pending() const { return used != 0 || overflow; }
    void discardPartial() { used = 0; overflow = true; }
private:
    char data[Capacity + 1] = {};
    size_t used = 0;
    bool overflow = false;
};
