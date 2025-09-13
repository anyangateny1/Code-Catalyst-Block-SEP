#ifndef BLOCK_GROWTH_H
#define BLOCK_GROWTH_H
#include <vector>
#include <string>
#include <unordered_map>
#include "block.h"

// 3D alias: [depth][height][width]
template<typename T>
using Vec3 = std::vector<std::vector<std::vector<T>>>;

// BlockGrowth encapsulates the "fit & grow" compression logic for a parent block
// over a sub-volume (model_slices). The tag_table maps single-char tags to labels.
class BlockGrowth {
public:
    BlockGrowth(const Vec3<char>& model_slices,
                const std::unordered_map<char, std::string>& tag_table);
    
    // Destructor to clean up raw array
    ~BlockGrowth();

    // Run the compression/growth algorithm on the given parent block.
    // Prints each fitted/grown block using Block::print_block(label).
    void run(Block parent_block);

private:
    // Inputs
    const Vec3<char>& model; // sub-volume: depth x height x width
    const std::unordered_map<char, std::string>& tag_table;
    
    // State for current run()
    Block parent_block{0,0,0,0,0,0,'\0'};
    int parent_x_end = 0, parent_y_end = 0, parent_z_end = 0;
    
    // Raw array for compressed tracking - MUCH faster than Vec3<bool>
    bool* compressed_raw = nullptr;
    int depth = 0, height = 0, width = 0; // Cached dimensions for fast indexing

    // Fast inline helper functions
    inline int get_index(int z, int y, int x) const {
        return z * height * width + y * width + x;
    }
    inline bool is_compressed(int z, int y, int x) const {
        return compressed_raw[get_index(z, y, x)];
    }
    inline char get_model(int z, int y, int x) const {
        return model[z][y][x];
    }

    // Helper functions
    bool all_compressed() const;
    char get_mode_of_uncompressed(const Block& blk) const;
    Block fit_block(char mode, int width, int height, int depth);
    void grow_block_iterative(Block& current, Block& best_block);
    bool window_is_all(char val,
                       int z0, int z1, int y0, int y1, int x0, int x1) const;
    bool window_is_all_uncompressed(int z0, int z1, int y0, int y1, int x0, int x1) const;
    void mark_compressed(int z0, int z1, int y0, int y1, int x0, int x1, bool v);
};
#endif // BLOCK_GROWTH_H