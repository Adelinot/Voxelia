
#pragma once

#include <vector>
#include <cstdint>

struct TerrainBlock {
    int x;
    int y;
    int z;
    int blockId;
};

std::vector<TerrainBlock> generateTerrain(
    int size,
    int depth,
    std::uint32_t* seedOut = nullptr
);

int getSurfaceHeight(
    const std::vector<TerrainBlock>& world,
    int x,
    int z
);