
#pragma once

#include "player.h"
#include "blocks.h"
#include "terrain.h"

#include <vector>
#include <unordered_map>

struct CrackOverlayState {
    bool active = false;

    int x = 0, y = 0, z = 0;
    int normalX = 0, normalY = 0, normalZ = 0;

    int frame = 0;
};

class BlockInteraction {
public:
    bool update(
        GLFWwindow* window,
        Player& player,
        std::vector<TerrainBlock>& world,
        const std::vector<BlockDefinition>& blocks,
        float deltaTime
    );

    const CrackOverlayState& getCrackOverlay() const {
        return crack;
    }

    int getSelectedBlockId() const {
        return selectedBlockId;
    }

private:
    int selectedBlockId = 1;

    bool leftWasDown = false;
    bool rightWasDown = false;

    float breakProgress = 0.0f;
    const float breakDuration = 1.0f;

    bool hasTarget = false;
    int targetX = 0, targetY = 0, targetZ = 0;

    CrackOverlayState crack;

    std::unordered_map<
        GridPosition,
        int,
        GridPositionHash
    > blockIndex;

    bool indexBuilt = false;

    void rebuildIndex(const std::vector<TerrainBlock>& world);
};