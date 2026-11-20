
#pragma once

#include "terrain.h"
#include <functional>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <unordered_set>
#include <cstddef>

struct GridPosition {
    int x, y, z;

    bool operator==(const GridPosition& other) const {
        return x == other.x &&
               y == other.y &&
               z == other.z;
    }
};

struct GridPositionHash {
    std::size_t operator()(const GridPosition& p) const {
        std::size_t h = std::hash<int>{}(p.x);

        h ^= std::hash<int>{}(p.y) +
             0x9e3779b9 + (h << 6) + (h >> 2);

        h ^= std::hash<int>{}(p.z) +
             0x9e3779b9 + (h << 6) + (h >> 2);

        return h;
    }
};

class Player {
public:
    Player(
        const std::vector<TerrainBlock>& world,
        const glm::vec3& spawnPosition
    );

    void update(GLFWwindow* window, float deltaTime);

    glm::mat4 getViewMatrix() const;

    void refreshWorld(const std::vector<TerrainBlock>& world);

    glm::vec3 getEyePosition() const;
    glm::vec3 getLookDirection() const;

    bool intersectsBlock(int x, int y, int z) const;

    void addSolidBlock(int x, int y, int z);
    void removeSolidBlock(int x, int y, int z);

private:
    glm::vec3 position;
    glm::vec3 spawnPosition;
    glm::vec3 velocity{0.0f};

    glm::vec3 front{0.0f, 0.0f, -1.0f};

    float yaw = -90.0f;
    float pitch = 0.0f;

    float movementSpeed = 5.0f;
    float sprintSpeed = 8.0f;
    float mouseSensitivity = 0.10f;

    float playerHeight = 1.8f;
    float playerRadius = 0.3f;
    float eyeHeight = 1.62f;

    bool firstMouse = true;
    bool grounded = false;

    double lastMouseX = 0.0;
    double lastMouseY = 0.0;

    std::unordered_set<
        GridPosition,
        GridPositionHash
    > solidBlocks;

    bool collidesAt(const glm::vec3& feetPosition) const;

    glm::vec3 cameraPosition() const;
};