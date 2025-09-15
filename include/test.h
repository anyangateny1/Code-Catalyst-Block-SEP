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
    
    // Initialize compressed mask - use bool for better cache efficiency
    compressed = Flat3D<bool>(parent_block.depth, parent_block.height, parent_block.width, false);
    
    // Process all uncompressed voxels using flood fill
    for (int z = 0; z < parent_block.depth; ++z) {
        for (int y = 0; y < parent_block.height; ++y) {
            for (int x = 0; x < parent_block.width; ++x) {
                if (!compressed.at(z, y, x)) {
                    Block block = flood_fill_block(x, y, z);
                    print_block(block);
                }
            }
        }
    }
}

Block BlockGrowth::flood_fill_block(int start_x, int start_y, int start_z) {
    char target_tag = model.at(start_z, start_y, start_x);
    
    // Collect all connected voxels of the same type
    vector<tuple<int, int, int>> connected_voxels;
    std::stack<tuple<int, int, int>> stack;
    stack.push({start_x, start_y, start_z});
    
    // Temporary visited array to avoid double-processing during flood fill
    Flat3D<bool> visited(parent_block.depth, parent_block.height, parent_block.width, false);
    
    while (!stack.empty()) {
        auto [x, y, z] = stack.top();
        stack.pop();
        
        // Skip if out of bounds, already processed, or different material
        if (!in_bounds(x, y, z) || compressed.at(z, y, x) || visited.at(z, y, x) ||
            model.at(z, y, x) != target_tag) {
            continue;
        }
        
        visited.at(z, y, x) = true;
        connected_voxels.push_back({x, y, z});
        
        // Add 6-connected neighbors to stack
        stack.push({x + 1, y, z});
        stack.push({x - 1, y, z});
        stack.push({x, y + 1, z});
        stack.push({x, y - 1, z});
        stack.push({x, y, z + 1});
        stack.push({x, y, z - 1});
    }
    
    // Create the best rectangular block from connected voxels
    Block result = create_rectangular_block(connected_voxels, target_tag);
    
    // Mark all voxels in the result block as compressed
    for (int z = result.z_offset; z < result.z_offset + result.depth; ++z) {
        for (int y = result.y_offset; y < result.y_offset + result.height; ++y) {
            for (int x = result.x_offset; x < result.x_offset + result.width; ++x) {
                compressed.at(z, y, x) = true;
            }
        }
    }
    
    return result;
}

Block BlockGrowth::create_rectangular_block(const vector<tuple<int, int, int>>& voxels, 
                                            char target_tag) {
    if (voxels.empty()) {
        throw std::runtime_error("Cannot create block from empty voxel set");
    }
    
    // Find bounding box of all connected voxels
    int min_x = std::get<0>(voxels[0]), max_x = min_x;
    int min_y = std::get<1>(voxels[0]), max_y = min_y;
    int min_z = std::get<2>(voxels[0]), max_z = min_z;
    
    for (const auto& [x, y, z] : voxels) {
        min_x = std::min(min_x, x);
        max_x = std::max(max_x, x);
        min_y = std::min(min_y, y);
        max_y = std::max(max_y, y);
        min_z = std::min(min_z, z);
        max_z = std::max(max_z, z);
    }
    
    int width = max_x - min_x + 1;
    int height = max_y - min_y + 1;
    int depth = max_z - min_z + 1;
    
    // Check if the bounding box is completely filled with target material
    bool is_solid_box = true;
    for (int z = min_z; z <= max_z && is_solid_box; ++z) {
        for (int y = min_y; y <= max_y && is_solid_box; ++y) {
            for (int x = min_x; x <= max_x && is_solid_box; ++x) {
                if (compressed.at(z, y, x) || model.at(z, y, x) != target_tag) {
                    is_solid_box = false;
                }
            }
        }
    }
    
    if (is_solid_box) {
        // Perfect rectangular block - use the full bounding box
        return Block(parent_block.x + min_x, parent_block.y + min_y, parent_block.z + min_z,
                     width, height, depth, target_tag, min_x, min_y, min_z);
    } else {
        // Irregular shape - find the largest rectangular sub-region
        int best_volume = 0;
        Block best_block(0, 0, 0, 1, 1, 1, target_tag);
        
        // Try different rectangular regions within the bounding box
        for (int z = min_z; z <= max_z; ++z) {
            for (int y = min_y; y <= max_y; ++y) {
                for (int x = min_x; x <= max_x; ++x) {
                    if (compressed.at(z, y, x) || model.at(z, y, x) != target_tag) continue;
                    
                    // Try to grow a rectangle from this point
                    int max_w = max_x - x + 1;
                    int max_h = max_y - y + 1;
                    int max_d = max_z - z + 1;
                    
                    for (int d = 1; d <= max_d; ++d) {
                        for (int h = 1; h <= max_h; ++h) {
                            for (int w = 1; w <= max_w; ++w) {
                                if (x + w - 1 > max_x || y + h - 1 > max_y || z + d - 1 > max_z) break;
                                
                                // Check if this rectangle is valid
                                bool valid = true;
                                for (int zz = z; zz < z + d && valid; ++zz) {
                                    for (int yy = y; yy < y + h && valid; ++yy) {
                                        for (int xx = x; xx < x + w && valid; ++xx) {
                                            if (compressed.at(zz, yy, xx) || model.at(zz, yy, xx) != target_tag) {
                                                valid = false;
                                            }
                                        }
                                    }
                                }
                                
                                if (valid && w * h * d > best_volume) {
                                    best_volume = w * h * d;
                                    best_block = Block(parent_block.x + x, parent_block.y + y, parent_block.z + z,
                                                       w, h, d, target_tag, x, y, z);
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // If no good rectangle found, just use a single voxel
        if (best_volume == 0) {
            auto [x, y, z] = voxels[0];
            best_block = Block(parent_block.x + x, parent_block.y + y, parent_block.z + z,
                               1, 1, 1, target_tag, x, y, z);
        }
        
        return best_block;
    }
}

void BlockGrowth::print_block(const Block& block) {
    auto it = tag_table.find(block.tag);
    const string& label = (it == tag_table.end()) ? string(1, block.tag) : it->second;
    block.print_block(label);
}