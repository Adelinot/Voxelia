
#include "player.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <algorithm>

Player::Player(
    const std::vector<TerrainBlock>& world,
    const glm::vec3& startPosition
) {
    position = startPosition;
    spawnPosition = startPosition;

    refreshWorld(world);
}

// --------------------------------------------------
// COLLISION DETECTION
// --------------------------------------------------

bool Player::collidesAt(
    const glm::vec3& feetPosition
) const {

    const float epsilon = 0.0001f;

    // Player bounding box in world coordinates.
    int minX = static_cast<int>(
        std::floor(feetPosition.x - playerRadius + 0.5f + epsilon)
    );

    int maxX = static_cast<int>(
        std::floor(feetPosition.x + playerRadius + 0.5f - epsilon)
    );

    int minY = static_cast<int>(
        std::floor(feetPosition.y + 0.5f + epsilon)
    );

    int maxY = static_cast<int>(
        std::floor(feetPosition.y + playerHeight + 0.5f - epsilon)
    );

    int minZ = static_cast<int>(
        std::floor(feetPosition.z - playerRadius + 0.5f + epsilon)
    );

    int maxZ = static_cast<int>(
        std::floor(feetPosition.z + playerRadius + 0.5f - epsilon)
    );

    // Check every block touched by the player.
    for (int x = minX; x <= maxX; x++) {
        for (int y = minY; y <= maxY; y++) {
            for (int z = minZ; z <= maxZ; z++) {

                if (solidBlocks.find({x, y, z})
                    != solidBlocks.end()) {
                    return true;
                }
            }
        }
    }

    return false;
}

// --------------------------------------------------
// CAMERA POSITION
// --------------------------------------------------

glm::vec3 Player::cameraPosition() const {
    return position + glm::vec3(0.0f, eyeHeight, 0.0f);
}

// --------------------------------------------------
// PLAYER UPDATE
// --------------------------------------------------

void Player::update(
    GLFWwindow* window,
    float deltaTime
) {

    // ----------------------------------------------
    // MOUSE LOOK
    // ----------------------------------------------

    double mouseX, mouseY;

    glfwGetCursorPos(window, &mouseX, &mouseY);

    if (firstMouse) {
        lastMouseX = mouseX;
        lastMouseY = mouseY;
        firstMouse = false;
    }

    float xOffset = static_cast<float>(
        mouseX - lastMouseX
    );

    float yOffset = static_cast<float>(
        lastMouseY - mouseY
    );

    lastMouseX = mouseX;
    lastMouseY = mouseY;

    yaw += xOffset * mouseSensitivity;
    pitch += yOffset * mouseSensitivity;

    pitch = std::clamp(pitch, -89.0f, 89.0f);

    // Calculate the direction the camera faces.
    glm::vec3 direction;

    direction.x =
        std::cos(glm::radians(yaw)) *
        std::cos(glm::radians(pitch));

    direction.y =
        std::sin(glm::radians(pitch));

    direction.z =
        std::sin(glm::radians(yaw)) *
        std::cos(glm::radians(pitch));

    front = glm::normalize(direction);

    // ----------------------------------------------
    // WASD MOVEMENT
    // ----------------------------------------------

    glm::vec3 forward = glm::normalize(
        glm::vec3(front.x, 0.0f, front.z)
    );

    glm::vec3 right = glm::normalize(
        glm::cross(forward, glm::vec3(0, 1, 0))
    );

    glm::vec3 movement(0.0f);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        movement += forward;

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        movement -= forward;

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        movement += right;

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        movement -= right;

    if (glm::length(movement) > 0.0f)
        movement = glm::normalize(movement);

    float speed = movementSpeed;

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        speed = sprintSpeed;

    glm::vec3 displacement =
        movement * speed * deltaTime;

    // Resolve X and Z separately so the player
    // can slide along walls.

    position.x += displacement.x;

    if (collidesAt(position))
        position.x -= displacement.x;

    position.z += displacement.z;

    if (collidesAt(position))
        position.z -= displacement.z;

    // ----------------------------------------------
    // JUMPING
    // ----------------------------------------------

    if (
        glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS
        && grounded
    ) {
        velocity.y = 7.0f;
        grounded = false;
    }

    // ----------------------------------------------
    // GRAVITY
    // ----------------------------------------------

    velocity.y -= 20.0f * deltaTime;

    float verticalMovement = velocity.y * deltaTime;

    position.y += verticalMovement;

    if (collidesAt(position)) {

        position.y -= verticalMovement;

        if (velocity.y < 0.0f)
            grounded = true;

        velocity.y = 0.0f;
    }
    else {
        grounded = false;
    }

    // Respawn if the player falls out of the world.
    if (position.y < -20.0f) {
        position = spawnPosition;
        velocity = glm::vec3(0.0f);
        grounded = false;
    }
}

// --------------------------------------------------
// VIEW MATRIX
// --------------------------------------------------

glm::mat4 Player::getViewMatrix() const {

    glm::vec3 camera = cameraPosition();

    return glm::lookAt(
        camera,
        camera + front,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
}

void Player::refreshWorld(
    const std::vector<TerrainBlock>& world
) {
    solidBlocks.clear();

    for (const TerrainBlock& block : world) {
        solidBlocks.insert({
            block.x,
            block.y,
            block.z
        });
    }
}

glm::vec3 Player::getEyePosition() const {
    return cameraPosition();
}

glm::vec3 Player::getLookDirection() const {
    return front;
}

bool Player::intersectsBlock(int x, int y, int z) const {
    const float epsilon = 0.0001f;

    return
        position.x + playerRadius > x - 0.5f + epsilon &&
        position.x - playerRadius < x + 0.5f - epsilon &&

        position.y + playerHeight > y - 0.5f + epsilon &&
        position.y < y + 0.5f - epsilon &&

        position.z + playerRadius > z - 0.5f + epsilon &&
        position.z - playerRadius < z + 0.5f - epsilon;
}

void Player::addSolidBlock(int x, int y, int z) {
    solidBlocks.insert({x, y, z});
}

void Player::removeSolidBlock(int x, int y, int z) {
    solidBlocks.erase({x, y, z});
}