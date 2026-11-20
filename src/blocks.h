#pragma once

#include <string>
#include <vector>
#include <array>

struct Tile {
    int column;
    int row;
};

struct BlockDefinition {
    int id;
    std::string name;

    // Front, back, left, right, top, bottom
    std::array<Tile, 6> faces;
};

bool loadBlockDatabase(
    const std::string& path,
    std::vector<BlockDefinition>& blocks
);