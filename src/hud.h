
#pragma once

#include "blocks.h"

#include <GL/glew.h>
#include <array>
#include <vector>

class HUD {
public:
    bool initialize();

    void render(
        int width,
        int height,
        GLuint atlasTexture,
        const std::vector<BlockDefinition>& blocks,
        int selectedBlockId
    );

    void cleanup();

private:
    GLuint program = 0;
    GLuint VAO = 0;
    GLuint VBO = 0;

    GLint screenSizeLoc = -1;
    GLint texturedLoc = -1;

    void drawQuad(
        float x,
        float y,
        float width,
        float height,
        const Tile* tile,
        const std::array<float, 4>& color
    );

    void drawDigit(
        int digit,
        float x,
        float y
    );
};
