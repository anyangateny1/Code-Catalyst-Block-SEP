#pragma once
#include <vector>
#include <cstddef>
#include <cstdint>

class Bitmask3D {
public:
    Bitmask3D(size_t depth, size_t height, size_t width)
        : d(depth), h(height), w(width), data(depth * height * width, 0) {}

    bool get(size_t z, size_t y, size_t x) const {
        return data[z * h * w + y * w + x];
    }
    void set(size_t z, size_t y, size_t x, bool value) {
        data[z * h * w + y * w + x] = value;
    }
private:
    size_t d, h, w;
    std::vector<uint8_t> data;
};