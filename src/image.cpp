#include "image.h"
#include "definitions.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"

ts_image img_logo;

// Simple helper function to load an image into a OpenGL texture with common settings
static bool _ts_load_texture(unsigned char* data, unsigned long size, GLuint* out_texture, int* out_width, int* out_height)
{
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load_from_memory(data, size, &image_width, &image_height, NULL, 4);
    if (image_data == NULL) {
		printf("Failed to load %s\n", stbi_failure_reason());
        return false;
	}

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;

    return true;
}

static ts_image _ts_load_image(unsigned char* data, unsigned long size) {
	int w = 0;
	int h = 0;
	GLuint id = 0;
	bool ret = _ts_load_texture(data, size, &id, &w, &h);

	return ts_image {id, w, h};
}

void ts_load_images() {
	int size = _binary_misc_logo_64_png_end - _binary_misc_logo_64_png_start;
	unsigned char* data = (unsigned char *)_binary_misc_logo_64_png_start;
	img_logo = _ts_load_image(data, size);
}