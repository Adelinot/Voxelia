
#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb_image.h"

#include <iostream>

GLuint loadTexture(const char* path) {

    int width = 0;
    int height = 0;
    int channels = 0;

    stbi_set_flip_vertically_on_load(true);

    unsigned char* pixels = stbi_load(
        path,
        &width,
        &height,
        &channels,
        STBI_rgb_alpha
    );

    if (!pixels) {
        std::cerr << "Texture loading failed: "
                  << path << "\n";

        std::cerr << stbi_failure_reason() << "\n";

        return 0;
    }

    GLuint texture = 0;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_NEAREST
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_NEAREST
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_CLAMP_TO_EDGE
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_CLAMP_TO_EDGE
    );

    stbi_image_free(pixels);

    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "Texture loaded: "
              << width << "x" << height << "\n";

    return texture;
}
