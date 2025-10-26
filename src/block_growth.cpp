#include "block_growth.h"
#include <stdexcept>
#include <algorithm>
#include <limits>

using std::string;
using std::unordered_map;

BlockGrowth::BlockGrowth(const Vec3<char>& model_slices,
                         const unordered_map<char, string>& tag_table)
    : model(model_slices), tag_table(tag_table) {}

void BlockGrowth::run(Block parent_block_) {
    parent_block = parent_block_;
    parent_x_end = parent_block.x_offset + parent_block.width;
    parent_y_end = parent_block.y_offset + parent_block.height;
    parent_z_end = parent_block.z_offset + parent_block.depth;

    compressed = Vec3<bool>(
        parent_block.depth,
        std::vector<std::vector<bool>>(parent_block.height, 
                                       std::vector<bool>(parent_block.width, false))
    );

    while (!all_compressed()) {
        // Find the largest possible block in uncompressed space
        Block best_block = fit_block('\0', 0, 0, 0);
        
        mark_compressed(best_block.z_offset, best_block.z_offset + best_block.depth,
                       best_block.y_offset, best_block.y_offset + best_block.height,
                       best_block.x_offset, best_block.x_offset + best_block.width, true);
        
        auto it = tag_table.find(best_block.tag);
        const string& label = (it == tag_table.end()) ? string(1, best_block.tag) : it->second;
        best_block.print_block(label);
    }
}

// Find the largest block by scanning all uncompressed positions
Block BlockGrowth::fit_block(char /*mode*/, int /*width*/, int /*height*/, int /*depth*/) {
    Block best_block(0, 0, 0, 0, 0, 0, '\0');
    int best_volume = 0;
    
    // Calculate max possible volume in remaining space
    int max_possible = (parent_x_end - parent_block.x_offset) * 
                       (parent_y_end - parent_block.y_offset) * 
                       (parent_z_end - parent_block.z_offset);
    
    for (int z = parent_block.z_offset; z < parent_z_end; ++z) {
        for (int y = parent_block.y_offset; y < parent_y_end; ++y) {
            for (int x = parent_block.x_offset; x < parent_x_end; ++x) {
                if (!compressed[z][y][x]) {
                    char tag = model[z][y][x];
                    
                    // Create seed block
                    Block current(parent_block.x + x, parent_block.y + y, parent_block.z + z,
                                1, 1, 1, tag, x, y, z);
                    
                    // Grow it maximally
                    grow_block(current, best_block);
                    
                    // Keep if it's the best so far
                    if (current.volume > best_volume) {
                        best_volume = current.volume;
                        best_block = current;
                        
                        // Early exit if we found a huge block (>25% of remaining space)
                        if (best_volume > max_possible / 4) {
                            return best_block;
                        }
                    }
                }
            }
        }
    }
    
    if (best_volume == 0) {
        throw std::runtime_error("No uncompressed block found");
    }
    
    return best_block;
}

// Grow a block maximally in X, then Y, then Z dimensions
void BlockGrowth::grow_block(Block& current, Block& /*best_block*/) {
    // Grow X dimension
    while (current.x_offset + current.width < parent_x_end) {
        int x_new = current.x_offset + current.width;
        bool can_grow = true;
        
        for (int z = current.z_offset; z < current.z_offset + current.depth && can_grow; ++z) {
            for (int y = current.y_offset; y < current.y_offset + current.height && can_grow; ++y) {
                if (compressed[z][y][x_new] || model[z][y][x_new] != current.tag) {
                    can_grow = false;
                }
            }
        }
        
        if (!can_grow) break;
        current.set_width(current.width + 1);
    }
    
    // Grow Y dimension
    while (current.y_offset + current.height < parent_y_end) {
        int y_new = current.y_offset + current.height;
        bool can_grow = true;
        
        for (int z = current.z_offset; z < current.z_offset + current.depth && can_grow; ++z) {
            for (int x = current.x_offset; x < current.x_offset + current.width && can_grow; ++x) {
                if (compressed[z][y_new][x] || model[z][y_new][x] != current.tag) {
                    can_grow = false;
                }
            }
        }
        
        if (!can_grow) break;
        current.set_height(current.height + 1);
    }
    
    // Grow Z dimension
    while (current.z_offset + current.depth < parent_z_end) {
        int z_new = current.z_offset + current.depth;
        bool can_grow = true;
        
        for (int y = current.y_offset; y < current.y_offset + current.height && can_grow; ++y) {
            for (int x = current.x_offset; x < current.x_offset + current.width && can_grow; ++x) {
                if (compressed[z_new][y][x] || model[z_new][y][x] != current.tag) {
                    can_grow = false;
                }
            }
        }
        
        if (!can_grow) break;
        current.set_depth(current.depth + 1);
    }
}

bool BlockGrowth::all_compressed() const {
    for (const auto& plane : compressed)
        for (const auto& row : plane)
            for (bool v : row)
                if (!v) return false;
    return true;
}

void BlockGrowth::mark_compressed(int z0, int z1, int y0, int y1, int x0, int x1, bool v) {
    for (int z = z0; z < z1; ++z)
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                compressed[z][y][x] = v;
}

// Helper: Get the most common uncompressed tag in a block region
char BlockGrowth::get_mode_of_uncompressed(const Block& blk) const {
    std::unordered_map<char, int> counts;
    
    for (int z = blk.z_offset; z < blk.z_offset + blk.depth; ++z) {
        for (int y = blk.y_offset; y < blk.y_offset + blk.height; ++y) {
            for (int x = blk.x_offset; x < blk.x_offset + blk.width; ++x) {
                if (!compressed[z][y][x]) {
                    counts[model[z][y][x]]++;
                }
            }
        }
    }
    
    char mode = '\0';
    int max_count = 0;
    for (const auto& pair : counts) {
        if (pair.second > max_count) {
            max_count = pair.second;
            mode = pair.first;
        }
    }
    
    return mode;
}

bool BlockGrowth::window_is_all(char val, int z0, int z1, int y0, int y1, int x0, int x1) const {
    for (int z = z0; z < z1; ++z)
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                if (model[z][y][x] != val)
                    return false;
    return true;
}

bool BlockGrowth::window_is_all_uncompressed(int z0, int z1, int y0, int y1, int x0, int x1) const {
    for (int z = z0; z < z1; ++z)
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                if (compressed[z][y][x])
                    return false;
    return true;
}