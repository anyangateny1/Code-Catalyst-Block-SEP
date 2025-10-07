#include "block_growth.h"
#include <stdexcept>
#include <algorithm>
#include "simd_utils.h"

using std::string;
using std::unordered_map;

BlockGrowth::BlockGrowth(const Flat3D<char>& model_slices,
                         const unordered_map<char, string>& tag_table)
    : model(model_slices), tag_table(tag_table) {}

void BlockGrowth::run(Block parent_block_) {
    parent_block = parent_block_;
    parent_x_end = parent_block.x_offset + parent_block.width;
    parent_y_end = parent_block.y_offset + parent_block.height;
    parent_z_end = parent_block.z_offset + parent_block.depth;

    // Initialise compressed mask to 0 (false)
    compressed = Flat3D<char>(parent_block.depth,
                              parent_block.height,
                              parent_block.width,
                              0);

    while (!all_compressed()) {
        char mode = get_mode_of_uncompressed(parent_block);
        int cube_size = std::min({parent_block.width, parent_block.height, parent_block.depth});
        Block b = fit_block(mode, cube_size, cube_size, cube_size);

        auto it = tag_table.find(b.tag);
        const string& label = (it == tag_table.end()) ? string(1, b.tag) : it->second;
        b.print_block(label);
    }
}

bool BlockGrowth::all_compressed() const {
    for (char v : compressed.data)
        if (v == 0) return false;
    return true;
}

char BlockGrowth::get_mode_of_uncompressed(const Block& blk) const {
    int z0 = blk.z_offset, z1 = blk.z_offset + blk.depth;
    int y0 = blk.y_offset, y1 = blk.y_offset + blk.height;
    int x0 = blk.x_offset, x1 = blk.x_offset + blk.width;

    int freq[256] = {0};
    for (int z = z0; z < z1; ++z) {
        for (int y = y0; y < y1; ++y) {
            const char* compRow = &compressed.data[(z * compressed.height + y) * compressed.width];
            const unsigned char* modelRow = reinterpret_cast<const unsigned char*>(&model.data[(z * model.height + y) * model.width]);
            int w = x1 - x0;
#if SEP_HAVE_SSE2
            int i = 0;
            // process 16-byte chunks
            for (; i + 16 <= w; i += 16) {
                const __m128i c = _mm_loadu_si128(reinterpret_cast<const __m128i*>(compRow + x0 + i));
                const __m128i zc = _mm_setzero_si128();
                const __m128i cmp = _mm_cmpeq_epi8(c, zc); // 0xFF where comp==0 (uncompressed)
                int mask = _mm_movemask_epi8(cmp);
                if (mask == 0) {
                    // all compressed -> skip
                } else if (mask == 0xFFFF) {
                    // all uncompressed -> tally all 16
                    const unsigned char* p = modelRow + x0 + i;
                    // unrolled tally
                    ++freq[p[0]]; ++freq[p[1]]; ++freq[p[2]]; ++freq[p[3]];
                    ++freq[p[4]]; ++freq[p[5]]; ++freq[p[6]]; ++freq[p[7]];
                    ++freq[p[8]]; ++freq[p[9]]; ++freq[p[10]]; ++freq[p[11]];
                    ++freq[p[12]]; ++freq[p[13]]; ++freq[p[14]]; ++freq[p[15]];
                } else {
                    // mixed -> check each lane
                    const unsigned char* p = modelRow + x0 + i;
                    const unsigned char* cbytes = reinterpret_cast<const unsigned char*>(compRow + x0 + i);
                    for (int k = 0; k < 16; ++k) {
                        if (cbytes[k] == 0) ++freq[p[k]];
                    }
                }
            }
            // remainder
            for (; i < w; ++i) {
                if (compRow[x0 + i] == 0) ++freq[modelRow[x0 + i]];
            }
#else
            for (int x = x0; x < x1; ++x) {
                if (compRow[x] == 0) ++freq[modelRow[x]];
            }
#endif
        }
    }

    char best = 0;
    int bestCount = -1;
    for (int i = 0; i < 256; ++i) {
        if (freq[i] > bestCount) {
            bestCount = freq[i];
            best = static_cast<char>(i);
        }
    }
    return best;
}

Block BlockGrowth::fit_block(char mode, int width, int height, int depth) {
    for (int z = parent_block.z; z < parent_block.z_end; ++z) {
        int z_off = z - parent_block.z;
        int z_end = z_off + depth;
        if (z_end > parent_z_end) break;

        for (int y = parent_block.y; y < parent_block.y_end; ++y) {
            int y_off = y - parent_block.y;
            int y_end = y_off + height;
            if (y_end > parent_y_end) break;

            for (int x = parent_block.x; x < parent_block.x_end; ++x) {
                int x_off = x - parent_block.x;
                int x_end = x_off + width;
                if (x_end > parent_x_end) break;

                // SIMD-accelerated row checks first to quickly reject non-uniform windows
                bool uniform = true;
                for (int zz = z_off; zz < z_end && uniform; ++zz) {
                    for (int yy = y_off; yy < y_end && uniform; ++yy) {
                        const char* row = &model.data[(zz * model.height + yy) * model.width];
                        if (!row_all_equal(row + x_off, static_cast<std::size_t>(x_end - x_off), mode)) uniform = false;
                    }
                }
                if (uniform) {
                    bool unc = true;
                    for (int zz = z_off; zz < z_end && unc; ++zz) {
                        for (int yy = y_off; yy < y_end && unc; ++yy) {
                            const char* rowc = &compressed.data[(zz * compressed.height + yy) * compressed.width];
                            if (!row_all_zero(rowc + x_off, static_cast<std::size_t>(x_end - x_off))) unc = false;
                        }
                    }
                    if (unc) {
                        Block b(x, y, z, width, height, depth, mode, x_off, y_off, z_off);
                        grow_block(b, b);
                        mark_compressed(z_off, z_off+b.depth, y_off, y_off+b.height, x_off, x_off+b.width, 1);
                        return b;
                    }
                }

            }
        }
    }

    if (width <= 1 || height <= 1 || depth <= 1) {
        throw std::runtime_error("No fitting block found at minimal size.");
    }
    return fit_block(mode, width - 1, height - 1, depth - 1);
}

bool BlockGrowth::window_is_all(char val,
                                int z0, int z1, int y0, int y1, int x0, int x1) const {
    for (int z = z0; z < z1; ++z) {
        for (int y = y0; y < y1; ++y) {
            const char* row = &model.data[(z * model.height + y) * model.width];
            if (!row_all_equal(row + x0, static_cast<std::size_t>(x1 - x0), val)) return false;
        }
    }
    return true;
}

bool BlockGrowth::window_is_all_uncompressed(int z0, int z1, int y0, int y1, int x0, int x1) const {
    for (int z = z0; z < z1; ++z) {
        for (int y = y0; y < y1; ++y) {
            const char* row = &compressed.data[(z * compressed.height + y) * compressed.width];
            if (!row_all_zero(row + x0, static_cast<std::size_t>(x1 - x0))) return false;
        }
    }
    return true;
}

void BlockGrowth::mark_compressed(int z0, int z1, int y0, int y1, int x0, int x1, char v) {
    for (int z = z0; z < z1; ++z)
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                compressed.at(z, y, x) = v;
}

void BlockGrowth::grow_block(Block& current, Block& best_block) {
    Block b = current;

    int x = b.x_offset, y = b.y_offset, z = b.z_offset;
    int x_end = x + b.width;
    int y_end = y + b.height;
    int z_end = z + b.depth;

    // Try +X growth
    if (x_end < parent_x_end) {
        bool ok = true;
        for (int zz = z; zz < z_end && ok; ++zz)
            for (int yy = y; yy < y_end && ok; ++yy) {
                if (model.at(zz, yy, x_end) != b.tag || compressed.at(zz, yy, x_end) != 0) ok = false;
            }
        if (ok) { b.set_width(b.width + 1); current = b; grow_block(current, best_block); if (current.volume > best_block.volume) best_block = current; b.set_width(b.width - 1); }
    }

    // Try +Y growth
    if (y_end < parent_y_end) {
        bool ok = true;
        for (int zz = z; zz < z_end && ok; ++zz)
            for (int xx = x; xx < x_end && ok; ++xx) {
                if (model.at(zz, y_end, xx) != b.tag || compressed.at(zz, y_end, xx) != 0) ok = false;
            }
        if (ok) { b.set_height(b.height + 1); current = b; grow_block(current, best_block); if (current.volume > best_block.volume) best_block = current; b.set_height(b.height - 1); }
    }

    // Try +Z growth
    if (z_end < parent_z_end) {
        bool ok = true;
        for (int yy = y; yy < y_end && ok; ++yy)
            for (int xx = x; xx < x_end && ok; ++xx) {
                if (model.at(z_end, yy, xx) != b.tag || compressed.at(z_end, yy, xx) != 0) ok = false;
            }
        if (ok) { b.set_depth(b.depth + 1); current = b; grow_block(current, best_block); if (current.volume > best_block.volume) best_block = current; b.set_depth(b.depth - 1); }
    }

    if (b.volume > best_block.volume)
        best_block = b;
}