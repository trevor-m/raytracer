#pragma once
#include "stb_image_write.h"

typedef unsigned char u08;

// Stores final image color values
class FrameBuffer {
private:
	u08* image;
	int width, height;

public:
	// Create a frame buffer with dimensions [width x height]
	FrameBuffer(int width, int height) {
		this->width = width;
		this->height = height;
		//allocate buffer
		image = new u08[width * height * 3];
		//clear to all black
		memset(image, 0, width * height * 3);
	}

	~FrameBuffer() {
		delete[] image;
	}

	// Get pointer to float[3] at pixel at (x,y)
	u08* getPixelPtr(int x, int y) {
		return (image + (y * width + x) * 3);
	}

	// Write out image to file
	void SaveToFile(const char* path) {
		stbi_write_bmp(path, width, height, 3, image);
	}
};