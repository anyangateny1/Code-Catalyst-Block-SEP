#include "block_growth.h"
#include <stdexcept>
#include <algorithm>
#include <stack>
#include <tuple>

using std::string;
using std::unordered_map;
using std::vector;
using std::tuple;

BlockGrowth::BlockGrowth(const Flat3D<char>& model_slices,
                         const unordered_map<char, string>& tag_table)
    : model(model_slices), tag_table(tag_table) {}

void BlockGrowth::run(Block parent_block_) {
    parent_block = parent_block_;

    // Initialize compressed mask - use unsigned char for bool-like
    compressed = Flat3D<unsigned char>(parent_block.depth, parent_block.height, parent_block.width, 0);
 
    // Process all uncompressed voxels using greedy block growth
    for (int z = 0; z < parent_block.depth; ++z) {
        for (int y = 0; y < parent_block.height; ++y) {
            for (int x = 0; x < parent_block.width; ++x) {
                if (!compressed.at(z, y, x)) {
                    Block block = flood_fill_block(x, y, z);
                    if (block.volume > 0) {  // Only print if valid block
                        print_block(block);
                    }
                }
            }
        }
    }
}

Block BlockGrowth::flood_fill_block(int start_x, int start_y, int start_z) {
    char target_tag = model.at(start_z, start_y, start_x);
    
    // Greedy growth: start from seed
    int x = start_x;
    int y = start_y;
    int z = start_z;

    // Grow width (x direction)
    int width = 1;
    while (in_bounds(x + width, y, z) &&
           model.at(z, y, x + width) == target_tag &&
           !compressed.at(z, y, x + width)) {
        width++;
    }

    // Grow height (y direction)
    int height = 1;
    bool can_grow = true;
    while (can_grow && in_bounds(x, y + height, z)) {
        for (int xx = x; xx < x + width; ++xx) {
            if (!in_bounds(xx, y + height, z) ||
                model.at(z, y + height, xx) != target_tag ||
                compressed.at(z, y + height, xx)) {
                can_grow = false;
                break;
            }
        }
        if (can_grow) height++;
    }

    // Grow depth (z direction)
    int depth = 1;
    can_grow = true;
    while (can_grow && in_bounds(x, y, z + depth)) {
        for (int yy = y; yy < y + height; ++yy) {
            for (int xx = x; xx < x + width; ++xx) {
                if (!in_bounds(xx, yy, z + depth) ||
                    model.at(z + depth, yy, xx) != target_tag ||
                    compressed.at(z + depth, yy, xx)) {
                    can_grow = false;
                    break;
                }
            }
            if (!can_grow) break;
        }
        if (can_grow) depth++;
    }

    // Create the block
    Block result(parent_block.x + x, parent_block.y + y, parent_block.z + z,
                 width, height, depth, target_tag, x, y, z);

    // Mark the rectangle as compressed
    for (int zz = z; zz < z + depth; ++zz) {
        for (int yy = y; yy < y + height; ++yy) {
            for (int xx = x; xx < x + width; ++xx) {
                compressed.at(zz, yy, xx) = 1;
            }
        }
    }
    
    return result;
}

void BlockGrowth::print_block(const Block& block) {
    auto it = tag_table.find(block.tag);
    const string& label = it->second;
    block.print_block(label);
}