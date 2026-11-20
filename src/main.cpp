
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "blocks.h"
#include "texture.h"
#include "terrain.h"
#include "player.h"
#include "interaction.h"
#include "hud.h"
#include <algorithm>

#include <unordered_set>
#include <functional>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <array>
#include <cstddef>

// --------------------------------------------------
// VERTEX DATA
// --------------------------------------------------

struct Vertex {
    float x, y, z;
    float nx, ny, nz;
    float u, v;
};

struct BlockPosition {
    int x, y, z;

    bool operator==(const BlockPosition& other) const {
        return x == other.x &&
               y == other.y &&
               z == other.z;
    }
};

struct BlockPositionHash {
    std::size_t operator()(const BlockPosition& p) const {
        std::size_t h = std::hash<int>{}(p.x);

        h ^= std::hash<int>{}(p.y)
             + 0x9e3779b9 + (h << 6) + (h >> 2);

        h ^= std::hash<int>{}(p.z)
             + 0x9e3779b9 + (h << 6) + (h >> 2);

        return h;
    }
};
// --------------------------------------------------
// SHADER LOADING
// --------------------------------------------------

std::string loadShader(const char* path) {
    std::ifstream file(path);

    if (!file) {
        std::cerr << "Cannot open shader: "
                  << path << "\n";
        return "";
    }

    std::stringstream stream;
    stream << file.rdbuf();

    return stream.str();
}

GLuint compileShader(
    GLenum type,
    const std::string& source
) {
    GLuint shader = glCreateShader(type);

    const char* code = source.c_str();

    glShaderSource(shader, 1, &code, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        char info[1024];

        glGetShaderInfoLog(
            shader,
            sizeof(info),
            nullptr,
            info
        );

        std::cerr << "Shader compilation error:\n"
                  << info << "\n";

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLuint createShaderProgram() {
    std::string vertexSource =
        loadShader("shaders/block.vert");

    std::string fragmentSource =
        loadShader("shaders/block.frag");

    if (vertexSource.empty() || fragmentSource.empty())
        return 0;

    GLuint vertex = compileShader(
        GL_VERTEX_SHADER,
        vertexSource
    );

    GLuint fragment = compileShader(
        GL_FRAGMENT_SHADER,
        fragmentSource
    );

    if (!vertex || !fragment) {
        if (vertex)
            glDeleteShader(vertex);

        if (fragment)
            glDeleteShader(fragment);

        return 0;
    }

    GLuint program = glCreateProgram();

    glAttachShader(program, vertex);
    glAttachShader(program, fragment);

    glLinkProgram(program);

    GLint success = 0;

    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success) {
        char info[1024];

        glGetProgramInfoLog(
            program,
            sizeof(info),
            nullptr,
            info
        );

        std::cerr << "Shader linking error:\n"
                  << info << "\n";
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    if (!success) {
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

// --------------------------------------------------
// TEXTURE ATLAS UV GENERATION
// --------------------------------------------------

std::array<float, 8> getTileUVs(const Tile& tile) {

    constexpr float atlasWidth = 48.0f;
    constexpr float atlasHeight = 64.0f;
    constexpr float tileSize = 16.0f;

    // Small inset avoids sampling neighbouring tiles.
    float u0 =
        (tile.column * tileSize + 0.5f)
        / atlasWidth;

    float u1 =
        ((tile.column + 1) * tileSize - 0.5f)
        / atlasWidth;

    // Image is flipped vertically by stb_image.
    float vTop =
        1.0f -
        (tile.row * tileSize + 0.5f)
        / atlasHeight;

    float vBottom =
        1.0f -
        ((tile.row + 1) * tileSize - 0.5f)
        / atlasHeight;

    return {
        u0, vBottom,
        u1, vBottom,
        u1, vTop,
        u0, vTop
    };
}

// --------------------------------------------------
// CUBE MESH GENERATION
// --------------------------------------------------

void appendCube(
    std::vector<Vertex>& vertices,
    std::vector<unsigned int>& indices,
    const BlockDefinition& block,
    const std::array<bool, 6>& visibleFaces,
    float worldX,
    float worldY,
    float worldZ
) {
    // Each face has four vertices.
    // Face order:
    // Front, Back, Left, Right, Top, Bottom.

    const float positions[6][4][3] = {

        // FRONT
        {
            {-0.5f, -0.5f,  0.5f},
            { 0.5f, -0.5f,  0.5f},
            { 0.5f,  0.5f,  0.5f},
            {-0.5f,  0.5f,  0.5f}
        },

        // BACK
        {
            { 0.5f, -0.5f, -0.5f},
            {-0.5f, -0.5f, -0.5f},
            {-0.5f,  0.5f, -0.5f},
            { 0.5f,  0.5f, -0.5f}
        },

        // LEFT
        {
            {-0.5f, -0.5f, -0.5f},
            {-0.5f, -0.5f,  0.5f},
            {-0.5f,  0.5f,  0.5f},
            {-0.5f,  0.5f, -0.5f}
        },

        // RIGHT
        {
            { 0.5f, -0.5f,  0.5f},
            { 0.5f, -0.5f, -0.5f},
            { 0.5f,  0.5f, -0.5f},
            { 0.5f,  0.5f,  0.5f}
        },

        // TOP
        {
            {-0.5f,  0.5f,  0.5f},
            { 0.5f,  0.5f,  0.5f},
            { 0.5f,  0.5f, -0.5f},
            {-0.5f,  0.5f, -0.5f}
        },

        // BOTTOM
        {
            {-0.5f, -0.5f, -0.5f},
            { 0.5f, -0.5f, -0.5f},
            { 0.5f, -0.5f,  0.5f},
            {-0.5f, -0.5f,  0.5f}
        }
    };

    const float normals[6][3] = {
        { 0,  0,  1},
        { 0,  0, -1},
        {-1,  0,  0},
        { 1,  0,  0},
        { 0,  1,  0},
        { 0, -1,  0}
    };

    for (int face = 0; face < 6; face++) {

        if (!visibleFaces[face])
            continue;

        auto uv = getTileUVs(block.faces[face]);

        unsigned int base =
            static_cast<unsigned int>(vertices.size());

        // Four vertices for this face.
        for (int i = 0; i < 4; i++) {

            Vertex vertex;

            vertex.x = positions[face][i][0] + worldX;
            vertex.y = positions[face][i][1] + worldY;
            vertex.z = positions[face][i][2] + worldZ;

            vertex.nx = normals[face][0];
            vertex.ny = normals[face][1];
            vertex.nz = normals[face][2];

            vertex.u = uv[i * 2];
            vertex.v = uv[i * 2 + 1];

            vertices.push_back(vertex);
        }

        // Two triangles make one face.
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);

        indices.push_back(base + 2);
        indices.push_back(base + 3);
        indices.push_back(base + 0);
    }
}


std::array<Vertex, 4> makeCrackQuad(
    const CrackOverlayState& crack
) {
    // Face order: Front, Back, Left, Right, Top, Bottom.
    int face = 0;

    if (crack.normalZ > 0) face = 0;
    else if (crack.normalZ < 0) face = 1;
    else if (crack.normalX < 0) face = 2;
    else if (crack.normalX > 0) face = 3;
    else if (crack.normalY > 0) face = 4;
    else if (crack.normalY < 0) face = 5;

    const float positions[6][4][3] = {
        {
            {-0.5f,-0.5f, 0.5f},
            { 0.5f,-0.5f, 0.5f},
            { 0.5f, 0.5f, 0.5f},
            {-0.5f, 0.5f, 0.5f}
        },
        {
            { 0.5f,-0.5f,-0.5f},
            {-0.5f,-0.5f,-0.5f},
            {-0.5f, 0.5f,-0.5f},
            { 0.5f, 0.5f,-0.5f}
        },
        {
            {-0.5f,-0.5f,-0.5f},
            {-0.5f,-0.5f, 0.5f},
            {-0.5f, 0.5f, 0.5f},
            {-0.5f, 0.5f,-0.5f}
        },
        {
            { 0.5f,-0.5f, 0.5f},
            { 0.5f,-0.5f,-0.5f},
            { 0.5f, 0.5f,-0.5f},
            { 0.5f, 0.5f, 0.5f}
        },
        {
            {-0.5f, 0.5f, 0.5f},
            { 0.5f, 0.5f, 0.5f},
            { 0.5f, 0.5f,-0.5f},
            {-0.5f, 0.5f,-0.5f}
        },
        {
            {-0.5f,-0.5f,-0.5f},
            { 0.5f,-0.5f,-0.5f},
            { 0.5f,-0.5f, 0.5f},
            {-0.5f,-0.5f, 0.5f}
        }
    };

    const float normals[6][3] = {
        {0,0,1}, {0,0,-1},
        {-1,0,0}, {1,0,0},
        {0,1,0}, {0,-1,0}
    };

    // Six visible stages in your 3x3 sprite sheet.
    const int columns[6] = {1, 2, 0, 1, 2, 0};
    const int rows[6] =    {0, 0, 1, 1, 1, 2};

    int frame = std::clamp(crack.frame, 0, 5);

    float u0 = (columns[frame] * 16.0f + 0.5f) / 48.0f;
    float u1 = ((columns[frame] + 1) * 16.0f - 0.5f) / 48.0f;

    float vTop = 1.0f - (rows[frame] * 16.0f + 0.5f) / 48.0f;
    float vBottom = 1.0f - ((rows[frame] + 1) * 16.0f - 0.5f) / 48.0f;

    float uv[4][2] = {
        {u0, vBottom},
        {u1, vBottom},
        {u1, vTop},
        {u0, vTop}
    };

    std::array<Vertex, 4> result;

    for (int i = 0; i < 4; i++) {
        Vertex& v = result[i];

        // Slightly move the overlay outside the block face.
        v.x = crack.x + positions[face][i][0] + normals[face][0] * 0.003f;
        v.y = crack.y + positions[face][i][1] + normals[face][1] * 0.003f;
        v.z = crack.z + positions[face][i][2] + normals[face][2] * 0.003f;

        v.nx = normals[face][0];
        v.ny = normals[face][1];
        v.nz = normals[face][2];

        v.u = uv[i][0];
        v.v = uv[i][1];
    }

    return result;
}

void buildWorldMesh(
    const std::vector<TerrainBlock>& world,
    const std::vector<BlockDefinition>& blocks,
    std::vector<Vertex>& vertices,
    std::vector<unsigned int>& indices
) {
    vertices.clear();
    indices.clear();

    // Build a fast lookup of occupied positions.
    std::unordered_set<BlockPosition, BlockPositionHash> occupied;

    occupied.reserve(world.size());

    for (const TerrainBlock& block : world) {
        occupied.insert({
            block.x,
            block.y,
            block.z
        });
    }

    // Check whether a neighbouring position contains a block.
    auto hasBlock = [&occupied](int x, int y, int z) {
        return occupied.find({x, y, z}) != occupied.end();
    };

    // Generate only visible faces.
    for (const TerrainBlock& terrainBlock : world) {

        const BlockDefinition* definition = nullptr;

        for (const auto& block : blocks) {
            if (block.id == terrainBlock.blockId) {
                definition = &block;
                break;
            }
        }

        if (!definition)
            continue;

        int x = terrainBlock.x;
        int y = terrainBlock.y;
        int z = terrainBlock.z;

        // Order matches appendCube:
        // Front, Back, Left, Right, Top, Bottom.

        std::array<bool, 6> visibleFaces = {
            !hasBlock(x,     y,     z + 1),
            !hasBlock(x,     y,     z - 1),
            !hasBlock(x - 1, y,     z),
            !hasBlock(x + 1, y,     z),
            !hasBlock(x,     y + 1, z),
            !hasBlock(x,     y - 1, z)
        };

        appendCube(
            vertices,
            indices,
            *definition,
            visibleFaces,
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(z)
        );
    }
}
// --------------------------------------------------
// MAIN
// --------------------------------------------------

int main() {

    // Initialize GLFW.
    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    glfwWindowHint(
        GLFW_OPENGL_PROFILE,
        GLFW_OPENGL_CORE_PROFILE
    );

    GLFWwindow* window = glfwCreateWindow(
        960,
        640,
        "VOXELIA - Block Texture Test",
        nullptr,
        nullptr
    );

    if (!window) {
        std::cerr << "Window creation failed\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Initialize GLEW.
    glewExperimental = GL_TRUE;

    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW initialization failed\n";

        glfwDestroyWindow(window);
        glfwTerminate();

        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // --------------------------------------------------
    // LOAD SHADERS
    // --------------------------------------------------

    GLuint shader = createShaderProgram();

    if (!shader) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // --------------------------------------------------
    // LOAD TEXTURE ATLAS
    // --------------------------------------------------

    GLuint atlasTexture =
        loadTexture("assets/textures/blocks.png");

    if (!atlasTexture) {
        glDeleteProgram(shader);

        glfwDestroyWindow(window);
        glfwTerminate();

        return -1;
    }

    GLuint crackTexture =
    loadTexture("assets/textures/crackoverlay.png");

    if (!crackTexture) {
        std::cerr << "Could not load crack overlay\n";
        glDeleteTextures(1, &atlasTexture);
        glDeleteProgram(shader);
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
}

    // --------------------------------------------------
    // LOAD BLOCK DATABASE
    // --------------------------------------------------

    std::vector<BlockDefinition> blocks;

    if (!loadBlockDatabase(
        "assets/data/blocks.json",
        blocks
    )) {
        glDeleteTextures(1, &atlasTexture);
        glDeleteProgram(shader);

        glfwDestroyWindow(window);
        glfwTerminate();

        return -1;
    }

    HUD hud;

    bool hudReady = hud.initialize();

    if (!hudReady){
        std::cerr << "hud failed";
    }

    // BUILD PROCEDURAL TERRAIN

    std::vector<TerrainBlock> world =
    generateTerrain(64, 16);

// Spawn near the centre, above the terrain.
    int spawnX = 32;
    int spawnZ = 32;

    int surfaceY = getSurfaceHeight(
    world,
    spawnX,
    spawnZ
);

    Player player(
    world,
    glm::vec3(
        (float)spawnX,
        (float)surfaceY + 3.0f,
        (float)spawnZ
    )
);

    BlockInteraction interaction;

    glfwSetInputMode(
    window,
    GLFW_CURSOR,
    GLFW_CURSOR_DISABLED);

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    buildWorldMesh(world, blocks, vertices, indices);
    std::cout << "World blocks: "
          << world.size() << "\n";

    std::cout << "MADE BY ADELINO ALESER T USING C++ AND OPENGL, THANKS\n";
    std::cout << "Found these programs very late so couldnt spend enough time. This is what i could do in 2 days. Hope you like it.\n Made a voxel engine in godot before. Hoping to see MipMaps program again";
    
    // --------------------------------------------------
    // SEND MESH TO GPU
    // --------------------------------------------------

    GLuint VAO, VBO, EBO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // Vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(Vertex),
        vertices.data(),
        GL_STATIC_DRAW
    );

    // Index buffer.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_STATIC_DRAW
    );

    // Position: attribute 0.
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(Vertex, x)
    );

    glEnableVertexAttribArray(0);

    // Normal: attribute 1.
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(Vertex, nx)
    );

    glEnableVertexAttribArray(1);

    // Texture coordinates: attribute 2.
    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(Vertex, u)
    );

    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    GLuint crackVAO, crackVBO, crackEBO;

    unsigned int crackIndices[] = {
    0, 1, 2,
    2, 3, 0};

    glGenVertexArrays(1, &crackVAO);
    glGenBuffers(1, &crackVBO);
    glGenBuffers(1, &crackEBO);

    glBindVertexArray(crackVAO);

    glBindBuffer(GL_ARRAY_BUFFER, crackVBO);

    glBufferData(
    GL_ARRAY_BUFFER,
    4 * sizeof(Vertex),
    nullptr,
    GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, crackEBO);

    glBufferData(
    GL_ELEMENT_ARRAY_BUFFER,
    sizeof(crackIndices),
    crackIndices,
    GL_STATIC_DRAW);

// Position
    glVertexAttribPointer(
    0, 3, GL_FLOAT, GL_FALSE,
    sizeof(Vertex), (void*)offsetof(Vertex, x));
    glEnableVertexAttribArray(0);

// Normal
    glVertexAttribPointer(
    1, 3, GL_FLOAT, GL_FALSE,
    sizeof(Vertex), (void*)offsetof(Vertex, nx));
    glEnableVertexAttribArray(1);

// Texture coordinates
    glVertexAttribPointer(
    2, 2, GL_FLOAT, GL_FALSE,
    sizeof(Vertex), (void*)offsetof(Vertex, u));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    // --------------------------------------------------
    // UNIFORM LOCATIONS
    // --------------------------------------------------

    GLint viewLoc =
        glGetUniformLocation(shader, "view");

    GLint projectionLoc =
        glGetUniformLocation(shader, "projection");

    GLint textureLoc =
        glGetUniformLocation(shader, "textureAtlas");


    std::cout << "\nVOXELIA started!\n";
    std::cout << "Blocks: " << blocks.size() << "\n";

    // --------------------------------------------------
    // GAME LOOP
    // --------------------------------------------------
    float lastFrame = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window)) {

        if (glfwGetKey(window, GLFW_KEY_ESCAPE)
            == GLFW_PRESS) {

            glfwSetWindowShouldClose(window, true);
        }

        float currentFrame = static_cast<float>(glfwGetTime());

        float deltaTime = currentFrame - lastFrame;

        lastFrame = currentFrame;

// Prevent huge physics steps after a pause.
        deltaTime = std::min(deltaTime, 0.05f);

        player.update(window, deltaTime);

        bool worldChanged = interaction.update(window, player, world, blocks, deltaTime);

        if (worldChanged) {

    // Rebuild the world mesh.
        buildWorldMesh(
        world,
        blocks,
        vertices,
        indices);

    // Upload the updated mesh to the GPU.
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(Vertex),
        vertices.data(),
        GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

        glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_DYNAMIC_DRAW);

        glBindVertexArray(0);}

        int width, height;

        glfwGetFramebufferSize(
            window,
            &width,
            &height
        );

        if (height == 0) {
            glfwPollEvents();
            continue;
        }

        glViewport(0, 0, width, height);

        glClearColor(
            0.45f,
            0.75f,
            0.9f,
            1.0f
        );

        glClear(
            GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT
        );

        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            static_cast<float>(width) /
                static_cast<float>(height),
            0.1f,
            250.0f
        );

        glUseProgram(shader);

        glm::mat4 view = player.getViewMatrix();

        glUniformMatrix4fv(viewLoc,1,GL_FALSE,&view[0][0]);

        glUniformMatrix4fv(
            projectionLoc,
            1,
            GL_FALSE,
            &projection[0][0]
        );

        // Bind atlas.
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, atlasTexture);

        glUniform1i(textureLoc, 0);

        // Draw all eight cubes in one call.
        glBindVertexArray(VAO);

        glDrawElements(
            GL_TRIANGLES,
            static_cast<GLsizei>(indices.size()),
            GL_UNSIGNED_INT,
            nullptr
        );
        
        const CrackOverlayState& crack =
    interaction.getCrackOverlay();

        if (crack.active) {

            auto quad = makeCrackQuad(crack);

            glUseProgram(shader);

    // Use the crack sprite sheet instead of blocks.png.
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, crackTexture);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Don't let the overlay write over the block's depth.
            glDepthMask(GL_FALSE);

            glBindVertexArray(crackVAO);

            glBindBuffer(GL_ARRAY_BUFFER, crackVBO);

            glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        sizeof(quad),
        quad.data());

            glDrawElements(
        GL_TRIANGLES,
        6,
        GL_UNSIGNED_INT,
        nullptr);

            glBindVertexArray(0);

            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);

        if (hudReady){
            hud.render(width, height, atlasTexture, blocks, interaction.getSelectedBlockId());
        };
}

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --------------------------------------------------
    // CLEANUP
    // --------------------------------------------------

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    glDeleteTextures(1, &atlasTexture);
    glDeleteProgram(shader);

    glDeleteVertexArrays(1, &crackVAO);
    glDeleteBuffers(1, &crackVBO);
    glDeleteBuffers(1, &crackEBO);


    glDeleteTextures(1, &crackTexture);

    if (hudReady){
        hud.cleanup();
    }
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}