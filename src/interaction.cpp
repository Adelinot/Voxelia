
#include "interaction.h"

#include <cmath>
#include <limits>
#include <algorithm>
#include <string>

void BlockInteraction::rebuildIndex(
    const std::vector<TerrainBlock>& world
) {
    blockIndex.clear();

    for (int i = 0; i < (int)world.size(); i++) {
        const auto& b = world[i];
        blockIndex[{b.x, b.y, b.z}] = i;
    }

    indexBuilt = true;
}

static bool raycastWorld(
    const glm::vec3& origin,
    const glm::vec3& direction,
    const std::unordered_map<GridPosition, int, GridPositionHash>& blockIndex,
    float maxDistance,
    int& hitIndex,
    int& placeX,
    int& placeY,
    int& placeZ
) {
    const float inf = std::numeric_limits<float>::infinity();

    int x = (int)std::floor(origin.x + 0.5f);
    int y = (int)std::floor(origin.y + 0.5f);
    int z = (int)std::floor(origin.z + 0.5f);

    int previousX = x, previousY = y, previousZ = z;

    int stepX = (direction.x > 0) - (direction.x < 0);
    int stepY = (direction.y > 0) - (direction.y < 0);
    int stepZ = (direction.z > 0) - (direction.z < 0);

    float deltaX = stepX ? std::abs(1.0f / direction.x) : inf;
    float deltaY = stepY ? std::abs(1.0f / direction.y) : inf;
    float deltaZ = stepZ ? std::abs(1.0f / direction.z) : inf;

    float maxX = stepX ?
        (x + (stepX > 0 ? 0.5f : -0.5f) - origin.x) / direction.x : inf;

    float maxY = stepY ?
        (y + (stepY > 0 ? 0.5f : -0.5f) - origin.y) / direction.y : inf;

    float maxZ = stepZ ?
        (z + (stepZ > 0 ? 0.5f : -0.5f) - origin.z) / direction.z : inf;

    for (int i = 0; i < 128; i++) {

        auto found = blockIndex.find({x, y, z});

        if (found != blockIndex.end()) {
            hitIndex = found->second;

            placeX = previousX;
            placeY = previousY;
            placeZ = previousZ;

            return true;
        }

        previousX = x;
        previousY = y;
        previousZ = z;

        float travelled;

        if (maxX < maxY && maxX < maxZ) {
            travelled = maxX;
            x += stepX;
            maxX += deltaX;
        }
        else if (maxY < maxZ) {
            travelled = maxY;
            y += stepY;
            maxY += deltaY;
        }
        else {
            travelled = maxZ;
            z += stepZ;
            maxZ += deltaZ;
        }

        if (travelled > maxDistance)
            break;
    }

    return false;
}

bool BlockInteraction::update(
    GLFWwindow* window,
    Player& player,
    std::vector<TerrainBlock>& world,
    const std::vector<BlockDefinition>& blocks,
    float deltaTime
) {
    if (!indexBuilt)
        rebuildIndex(world);

    // Number keys select blocks by ID 1-8.
    for (int i = 0; i < 8; i++) {
        if (glfwGetKey(window, GLFW_KEY_1 + i) == GLFW_PRESS) {
            for (const auto& block : blocks) {
                if (block.id == i + 1) {
                    selectedBlockId = block.id;

                    std::string title =
                        "VOXELIA - Selected: " + block.name;

                    glfwSetWindowTitle(window, title.c_str());
                    break;
                }
            }
        }
    }

    bool leftDown =
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

    bool rightDown =
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

    bool rightPressed = rightDown && !rightWasDown;

    leftWasDown = leftDown;
    rightWasDown = rightDown;

    int hitIndex = -1;
    int placeX = 0, placeY = 0, placeZ = 0;

    bool hit = raycastWorld(
        player.getEyePosition(),
        player.getLookDirection(),
        blockIndex,
        6.0f,
        hitIndex,
        placeX,
        placeY,
        placeZ
    );

    // ------------------------------------------
    // HOLD LEFT CLICK TO BREAK
    // ------------------------------------------

    if (leftDown && hit) {

        TerrainBlock target = world[hitIndex];

        if (!hasTarget ||
            target.x != targetX ||
            target.y != targetY ||
            target.z != targetZ) {

            breakProgress = 0.0f;

            targetX = target.x;
            targetY = target.y;
            targetZ = target.z;

            hasTarget = true;
        }

        breakProgress += std::clamp(deltaTime, 0.0f, 0.05f);

        crack.active = true;

        crack.x = targetX;
        crack.y = targetY;
        crack.z = targetZ;

        crack.normalX = placeX - targetX;
        crack.normalY = placeY - targetY;
        crack.normalZ = placeZ - targetZ;

        crack.frame = std::min(
            5,
            (int)(breakProgress / breakDuration * 6.0f)
        );

        if (breakProgress >= breakDuration) {

            GridPosition removed{
                target.x, target.y, target.z
            };

            int lastIndex = (int)world.size() - 1;

            // Remove block efficiently using swap-remove.
            player.removeSolidBlock(
                target.x, target.y, target.z
            );

            if (hitIndex != lastIndex) {
                world[hitIndex] = world[lastIndex];

                const auto& moved = world[hitIndex];

                blockIndex[{
                    moved.x, moved.y, moved.z
                }] = hitIndex;
            }

            world.pop_back();
            blockIndex.erase(removed);

            breakProgress = 0.0f;
            hasTarget = false;
            crack.active = false;

            return true;
        }
    }
    else {
        breakProgress = 0.0f;
        hasTarget = false;
        crack.active = false;
    }

    // ------------------------------------------
    // RIGHT CLICK TO PLACE
    // ------------------------------------------

    if (rightPressed && hit) {

        GridPosition destination{
            placeX, placeY, placeZ
        };

        if (blockIndex.find(destination) != blockIndex.end())
            return false;

        if (player.intersectsBlock(placeX, placeY, placeZ))
            return false;

        int newIndex = (int)world.size();

        world.push_back({
            placeX,
            placeY,
            placeZ,
            selectedBlockId
        });

        blockIndex[destination] = newIndex;

        player.addSolidBlock(placeX, placeY, placeZ);

        return true;
    }

    return false;
}