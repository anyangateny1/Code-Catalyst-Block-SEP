#ifndef BLOCK_GROWTH_H
#define BLOCK_GROWTH_H

#include "block.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <vector>
#include <stdexcept>

// Flattened 3D container: [depth][height][width]
template <typename T>
class Flat3D {
public:
    int depth, height, width;
    std::vector<T> data;

    Flat3D() : depth(0), height(0), width(0) {}
    Flat3D(int d, int h, int w, T init = T()) : depth(d), height(h), width(w), data(d * h * w, init) {}

    inline T& at(int z, int y, int x) {
        return data[(z * height + y) * width + x];
    }

    inline const T& at(int z, int y, int x) const {
        return data[(z * height + y) * width + x];
    }
};

// A view window of a larger Flat3D instance
template <typename T>
class Flat3DView {
private:
    T* base_ptr;
    int base_depth, base_height, base_width;
    int y0, x0;

public:
    int depth, height, width;

    Flat3DView(Flat3D<T>& base,
               int depth_, int y_start, int y_end,
               int x_start, int x_end)
        : base_ptr(base.data.data()),
          base_depth(base.depth),
          base_height(base.height),
          base_width(base.width),
          y0(y_start),
          x0(x_start),
          depth(depth_),
          height(y_end - y_start),
          width(x_end - x_start)
    {
        if (depth_ > base_depth)
            throw std::out_of_range("Flat3DView: depth exceeds base");
        if (y_start < 0 || y_end > base_height ||
            x_start < 0 || x_end > base_width)
            throw std::out_of_range("Flat3DView: region exceeds base bounds");
    }

    inline T& at(int z, int y, int x) {
        if (z < 0 || z >= depth || y < 0 || y >= height || x < 0 || x >= width)
            throw std::out_of_range("Flat3DView::at() out of range");
        return base_ptr[(z * base_height + (y + y0)) * base_width + (x + x0)];
    }

    inline const T& at(int z, int y, int x) const {
        if (z < 0 || z >= depth || y < 0 || y >= height || x < 0 || x >= width)
            throw std::out_of_range("Flat3DView::at() out of range");
        return base_ptr[(z * base_height + (y + y0)) * base_width + (x + x0)];
    }
};



// BlockGrowth encapsulates the "fit & grow" compression logic for a parent block
// over a sub-volume (model_slices). The tag_table maps single-char tags to labels.
class BlockGrowth {
public:
    BlockGrowth(const Flat3DView<char>& model_slices, const std::unordered_map<char, std::string>& tag_table);

    void run(Block parent_block);

    static std::mutex print_mutex;

private:
    const Flat3DView<char>& model;
    const std::unordered_map<char, std::string>& tag_table;

    Block parent_block{0, 0, 0, 0, 0, 0, '\0'};
    int parent_x_end = 0, parent_y_end = 0, parent_z_end = 0;

    // Tracks which cells in 'model' have been compressed (0 = false, 1 = true)
    Flat3D<char> compressed;

    // Store a set of the uncompressed coordinates
    std::set<uint64_t> uncompressed_coords;
    uint64_t encode_coord(int x, int y, int z) const {
        return (static_cast<uint64_t>(z) << 32) | 
               (static_cast<uint64_t>(y) << 16) | 
               static_cast<uint64_t>(x);
    }
    std::tuple<int, int, int> decode_coord(uint64_t encoded) const {
        int x = encoded & 0xFFFF;
        int y = (encoded >> 16) & 0xFFFF;
        int z = (encoded >> 32) & 0xFFFF;
        return {x, y, z};
    }

    bool all_compressed() const;
    char get_mode_of_uncompressed(const Block& blk) const;

    Block fit_block(int width, int height, int depth);
    void grow_block(Block& b);

    bool window_is_all(char val, int z0, int z1, int y0, int y1, int x0, int x1) const;
    bool window_is_all_uncompressed(int z0, int z1, int y0, int y1, int x0, int x1) const;
    void mark_compressed(int z0, int z1, int y0, int y1, int x0, int x1);
};

#endif  // BLOCK_GROWTH_H
