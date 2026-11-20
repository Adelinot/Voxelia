#include "blocks.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>
#include <unordered_set>
#include <utility>

using json = nlohmann::json;

static bool readTile(
    const json& value,
    Tile& tile
) {
    if (!value.is_array() || value.size() != 2)
        return false;

    if (!value[0].is_number_integer() ||
        !value[1].is_number_integer())
        return false;

    tile.column = value[0].get<int>();
    tile.row = value[1].get<int>();

    // Our atlas has 3 columns and 4 rows.
    return tile.column >= 0 && tile.column < 3 &&
           tile.row >= 0 && tile.row < 4;
}

bool loadBlockDatabase(
    const std::string& path,
    std::vector<BlockDefinition>& blocks
) {
    std::ifstream file(path);

    if (!file) {
        std::cerr << "Cannot open: " << path << "\n";
        return false;
    }

    json data;

    try {
        file >> data;
    }
    catch (const std::exception& e) {
        std::cerr << "JSON error: " << e.what() << "\n";
        return false;
    }

    if (!data.is_array()) {
        std::cerr << "Block database must be an array\n";
        return false;
    }

    std::vector<BlockDefinition> loaded;
    std::unordered_set<int> usedIDs;

    for (const auto& item : data) {
        try {
            BlockDefinition block{};

            block.id = item.at("id").get<int>();
            block.name = item.at("name").get<std::string>();

            if (block.id <= 0 || block.name.empty()) {
                std::cerr << "Invalid block ID or name\n";
                return false;
            }

            if (!usedIDs.insert(block.id).second) {
                std::cerr << "Duplicate block ID: "
                          << block.id << "\n";
                return false;
            }

            if (item.contains("all")) {
                Tile tile;

                if (!readTile(item.at("all"), tile)) {
                    std::cerr << "Invalid texture for "
                              << block.name << "\n";
                    return false;
                }

                block.faces.fill(tile);
            }
            else {
                Tile side, top, bottom;

                if (!readTile(item.at("side"), side) ||
                    !readTile(item.at("top"), top) ||
                    !readTile(item.at("bottom"), bottom)) {

                    std::cerr << "Invalid face textures for "
                              << block.name << "\n";
                    return false;
                }

                for (int i = 0; i < 4; i++)
                    block.faces[i] = side;

                block.faces[4] = top;
                block.faces[5] = bottom;
            }

            loaded.push_back(block);
        }
        catch (const std::exception& e) {
            std::cerr << "Invalid block entry: "
                      << e.what() << "\n";
            return false;
        }
    }

    if (loaded.empty()) {
        std::cerr << "No blocks found\n";
        return false;
    }

    blocks = std::move(loaded);

    std::cout << "Loaded "
              << blocks.size()
              << " block definitions\n";

    return true;
}