#ifndef BLOCK_GROWTH_H
#define BLOCK_GROWTH_H

#include "block.h"
#include <string>
#include <unordered_map>
#include <vector>

// Flattened 3D container: [depth][height][width]
template <typename T>
class Flat3D {
public:
    int depth, height, width;
    std::vector<T> data;

    Flat3D() : depth(0), height(0), width(0) {}
    
    Flat3D(int d, int h, int w, T init = T()) 
        : depth(d), height(h), width(w), data(d * h * w, init) {}

    // Inline for better performance
    inline T& at(int z, int y, int x) {
        return data[(z * height + y) * width + x];
    }

    inline const T& at(int z, int y, int x) const {
        return data[(z * height + y) * width + x];
    }
};

// BlockGrowth encapsulates the flood-fill compression logic for a parent block
// over a sub-volume (model_slices). The tag_table maps single-char tags to labels.
class BlockGrowth {
public:
    BlockGrowth(const Flat3D<char>& model_slices, 
                const std::unordered_map<char, std::string>& tag_table);
    
    void run(Block parent_block);

private:
    const Flat3D<char>& model;
    const std::unordered_map<char, std::string>& tag_table;
    Block parent_block{0, 0, 0, 0, 0, 0, '\0'};
    
    // Use bool for better memory efficiency and cache performance
    Flat3D<unsigned char> compressed;
    
    // Core greedy growth algorithm to form a rectangular block
    Block flood_fill_block(int start_x, int start_y, int start_z);
    
    // Helper to print block with proper label lookup
    void print_block(const Block& block);
    
    // Check if coordinates are within parent block bounds
    inline bool in_bounds(int x, int y, int z) const {
        return x >= 0 && x < parent_block.width &&
               y >= 0 && y < parent_block.height &&
               z >= 0 && z < parent_block.depth;
    }
};

#endif // BLOCK_GROWTH_H