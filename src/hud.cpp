
#include "hud.h"

#include <iostream>

static GLuint compileHUDShader(
    GLenum type,
    const char* source
) {
    GLuint shader = glCreateShader(type);

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        char info[1024];

        glGetShaderInfoLog(
            shader, 1024, nullptr, info
        );

        std::cerr << "HUD shader error: "
                  << info << "\n";

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

// --------------------------------------------------
// INITIALIZE
// --------------------------------------------------

bool HUD::initialize() {

    const char* vertexSource = R"(
        #version 330 core

        layout(location = 0) in vec2 aPosition;
        layout(location = 1) in vec2 aUV;
        layout(location = 2) in vec4 aColor;

        out vec2 TexCoord;
        out vec4 Color;

        uniform vec2 screenSize;

        void main() {
            vec2 position = vec2(
                aPosition.x / screenSize.x * 2.0 - 1.0,
                1.0 - aPosition.y / screenSize.y * 2.0
            );

            gl_Position = vec4(position, 0.0, 1.0);

            TexCoord = aUV;
            Color = aColor;
        }
    )";

    const char* fragmentSource = R"(
        #version 330 core

        in vec2 TexCoord;
        in vec4 Color;

        out vec4 FragColor;

        uniform sampler2D atlas;
        uniform int useTexture;

        void main() {
            if (useTexture == 1) {
                FragColor = texture(atlas, TexCoord) * Color;
            } else {
                FragColor = Color;
            }
        }
    )";

    GLuint vertex = compileHUDShader(
        GL_VERTEX_SHADER, vertexSource
    );

    GLuint fragment = compileHUDShader(
        GL_FRAGMENT_SHADER, fragmentSource
    );

    if (!vertex || !fragment) {
        if (vertex) glDeleteShader(vertex);
        if (fragment) glDeleteShader(fragment);
        return false;
    }

    program = glCreateProgram();

    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    GLint success = 0;

    glGetProgramiv(program, GL_LINK_STATUS, &success);

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    if (!success) {
        char info[1024];

        glGetProgramInfoLog(
            program, 1024, nullptr, info
        );

        std::cerr << "HUD linking error: "
                  << info << "\n";

        glDeleteProgram(program);
        program = 0;
        return false;
    }

    screenSizeLoc =
        glGetUniformLocation(program, "screenSize");

    texturedLoc =
        glGetUniformLocation(program, "useTexture");

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        6 * 8 * sizeof(float),
        nullptr,
        GL_DYNAMIC_DRAW
    );

    constexpr GLsizei stride = 8 * sizeof(float);

    glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE,
        stride, (void*)0
    );
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE,
        stride, (void*)(2 * sizeof(float))
    );
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(
        2, 4, GL_FLOAT, GL_FALSE,
        stride, (void*)(4 * sizeof(float))
    );
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    std::cout << "HUD initialized\n";

    return true;
}

// --------------------------------------------------
// DRAW QUAD
// --------------------------------------------------

void HUD::drawQuad(
    float x,
    float y,
    float width,
    float height,
    const Tile* tile,
    const std::array<float, 4>& color
) {
    float u0 = 0.0f;
    float u1 = 0.0f;
    float v0 = 0.0f;
    float v1 = 0.0f;

    bool textured = tile != nullptr;

    if (textured) {
        u0 = (tile->column * 16.0f + 0.5f) / 48.0f;
        u1 = ((tile->column + 1) * 16.0f - 0.5f) / 48.0f;

        v0 = 1.0f - (tile->row * 16.0f + 0.5f) / 64.0f;
        v1 = 1.0f - ((tile->row + 1) * 16.0f - 0.5f) / 64.0f;
    }

    float vertices[] = {
        // x, y, u, v, r, g, b, a

        x, y, u0, v0,
        color[0], color[1], color[2], color[3],

        x, y + height, u0, v1,
        color[0], color[1], color[2], color[3],

        x + width, y + height, u1, v1,
        color[0], color[1], color[2], color[3],

        x, y, u0, v0,
        color[0], color[1], color[2], color[3],

        x + width, y + height, u1, v1,
        color[0], color[1], color[2], color[3],

        x + width, y, u1, v0,
        color[0], color[1], color[2], color[3]
    };

    glUniform1i(texturedLoc, textured ? 1 : 0);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        sizeof(vertices),
        vertices
    );

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

// --------------------------------------------------
// DRAW SLOT NUMBER
// --------------------------------------------------

void HUD::drawDigit(
    int digit,
    float x,
    float y
) {
    static const char* digits[8][5] = {
        {"010","110","010","010","111"},
        {"111","001","111","100","111"},
        {"111","001","111","001","111"},
        {"101","101","111","001","001"},
        {"111","100","111","001","111"},
        {"111","100","111","101","111"},
        {"111","001","010","010","010"},
        {"111","101","111","101","111"}
    };

    if (digit < 1 || digit > 8)
        return;

    const float pixel = 1.6f;

    std::array<float, 4> color = {
        0.95f, 0.95f, 1.0f, 1.0f
    };

    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 3; col++) {
            if (digits[digit - 1][row][col] == '1') {
                drawQuad(
                    x + col * pixel,
                    y + row * pixel,
                    pixel,
                    pixel,
                    nullptr,
                    color
                );
            }
        }
    }
}

// --------------------------------------------------
// RENDER HUD
// --------------------------------------------------

void HUD::render(
    int screenWidth,
    int screenHeight,
    GLuint atlasTexture,
    const std::vector<BlockDefinition>& blocks,
    int selectedBlockId
) {
    if (!program || screenWidth < 460 || screenHeight < 200)
        return;

    GLboolean depthEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean cullEnabled = glIsEnabled(GL_CULL_FACE);
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);

    glBlendFunc(
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA
    );

    glUseProgram(program);

    glUniform2f(
        screenSizeLoc,
        (float)screenWidth,
        (float)screenHeight
    );

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlasTexture);

    glUniform1i(
        glGetUniformLocation(program, "atlas"),
        0
    );

    glBindVertexArray(VAO);

    std::array<float, 4> panelColor = {
        0.035f, 0.045f, 0.065f, 0.88f
    };

    std::array<float, 4> slotColor = {
        0.12f, 0.14f, 0.18f, 0.95f
    };

    std::array<float, 4> selectedColor = {
        0.85f, 0.62f, 0.20f, 1.0f
    };

    // --------------------------------------------------
    // HOTBAR POSITION
    // --------------------------------------------------

    const float slotSize = 48.0f;
    const float gap = 5.0f;

    const float totalWidth =
        8.0f * slotSize + 7.0f * gap;

    float startX =
        (screenWidth - totalWidth) * 0.5f;

    float startY =
        screenHeight - 70.0f;

    // Background panel.
    drawQuad(
        startX - 12.0f,
        startY - 10.0f,
        totalWidth + 24.0f,
        68.0f,
        nullptr,
        panelColor
    );

    // --------------------------------------------------
    // EIGHT HOTBAR SLOTS
    // --------------------------------------------------

    for (int i = 0; i < 8; i++) {

        float x = startX + i * (slotSize + gap);

        int blockID = i + 1;

        bool selected = blockID == selectedBlockId;

        // Find block by ID.
        const BlockDefinition* block = nullptr;

        for (const auto& entry : blocks) {
            if (entry.id == blockID) {
                block = &entry;
                break;
            }
        }

        if (selected) {
            drawQuad(
                x - 3.0f,
                startY - 3.0f,
                slotSize + 6.0f,
                slotSize + 6.0f,
                nullptr,
                selectedColor
            );
        }

        drawQuad(
            x,
            startY,
            slotSize,
            slotSize,
            nullptr,
            slotColor
        );

        // Show the block's texture.
        if (block) {
            // Grass uses its top face as the icon.
            Tile icon = block->name == "grass"
                ? block->faces[4]
                : block->faces[0];

            drawQuad(
                x + 9.0f,
                startY + 9.0f,
                30.0f,
                30.0f,
                &icon,
                {1.0f, 1.0f, 1.0f, 1.0f}
            );
        }

        drawDigit(
            i + 1,
            x + 4.0f,
            startY + 4.0f
        );
    }

    // --------------------------------------------------
    // CROSSHAIR
    // --------------------------------------------------

    float centerX = screenWidth * 0.5f;
    float centerY = screenHeight * 0.5f;

    std::array<float, 4> black = {
        0.0f, 0.0f, 0.0f, 0.8f
    };

    std::array<float, 4> white = {
        1.0f, 1.0f, 1.0f, 0.95f
    };

    drawQuad(
        centerX - 2.5f,
        centerY - 10.0f,
        5.0f,
        20.0f,
        nullptr,
        black
    );

    drawQuad(
        centerX - 10.0f,
        centerY - 2.5f,
        20.0f,
        5.0f,
        nullptr,
        black
    );

    drawQuad(
        centerX - 1.0f,
        centerY - 8.0f,
        2.0f,
        16.0f,
        nullptr,
        white
    );

    drawQuad(
        centerX - 8.0f,
        centerY - 1.0f,
        16.0f,
        2.0f,
        nullptr,
        white
    );

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (!blendEnabled)
        glDisable(GL_BLEND);

    if (depthEnabled)
        glEnable(GL_DEPTH_TEST);

    if (cullEnabled)
        glEnable(GL_CULL_FACE);
}

// --------------------------------------------------
// CLEANUP
// --------------------------------------------------

void HUD::cleanup() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(program);

    VAO = 0;
    VBO = 0;
    program = 0;
}
