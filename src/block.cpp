#include "block.h"
#include <iostream>

Block::Block(int x_, int y_, int z_,
             int w_, int h_, int d_,
             char tag_,
             int x_off, int y_off, int z_off)
    : x(x_), y(y_), z(z_),
      x_offset(x_off), y_offset(y_off), z_offset(z_off),
      width(w_), height(h_), depth(d_), tag(tag_) {  
      update_derived_values();
    }

void Block::set_width(int w) {
    width = w;
    update_derived_values();
}

void Block::set_height(int h) {
    height = h;
    update_derived_values();
}

void Block::set_depth(int d) {
    depth = d;
    update_derived_values();
}
// separated it from the constructor block it was handling to many processes.
void Block::update_derived_values() {
    volume = width * height * depth;
    x_end = x + width;
    y_end = y + height;
    z_end = z + depth;
}

void Block::print_block(const std::string& label) const {
    std::cout << x << "," << y << "," << z << ","
              << width << "," << height << "," << depth << ","
              << label << "\n";
}