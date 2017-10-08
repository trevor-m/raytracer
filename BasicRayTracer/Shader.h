#pragma once
#include "Material.h"
#include "Noise.h"
#include "stb_image.h"

// A color shader allows you to edit materials after an intersection
class ColorShader {
public:
	virtual void Shade(HitData& hitData, Material& out) = 0;
};

// An intersection shader allows you to choose whether an intersection occurs
class IntersectionShader {
public:
	// Return true to accept the intersection or false to reject it
	virtual bool Shade(const HitData& hitData) = 0;
};

// Checkerboard pattern
class CheckerBoardColorShader : public ColorShader {
protected:
	float checkSizeU, checkSizeV;
public:

	CheckerBoardColorShader(float checkSizeU, float checkSizeV) {
		this->checkSizeU = checkSizeU;
		this->checkSizeV = checkSizeV;
	}

	void Shade(HitData& hitData, Material& out) {
		if ((int)(floor(hitData.u*checkSizeU) + floor(hitData.v*checkSizeV)) % 2 == 0) {
			out.diffColor = Vector3(0.7, 0.7, 0.7);
		}
		else {
			out.diffColor = Vector3(0.1, 0.1, 0.1);
		}
	}
};

class CheckerBoardIntersectionShader : public IntersectionShader {
protected:
	float checkSizeU, checkSizeV;
public:

	CheckerBoardIntersectionShader(float checkSizeU, float checkSizeV) {
		this->checkSizeU = checkSizeU;
		this->checkSizeV = checkSizeV;
	}

	bool Shade(const HitData& hitData) {
		if ((int)(floor(hitData.u*checkSizeU) + floor(hitData.v*checkSizeV)) % 2 == 0) {
			return true;
		}
		else {
			return false;
		}
	}
};

// Load and use a texture to color the object
class TextureColorShader : public ColorShader {
protected:
	unsigned char* data;
	int width, height;

public:
	TextureColorShader(const char* filepath) {
		int n;
		data = stbi_load(filepath, &width, &height, &n, 3);
		if (data == NULL) {
			printf("Error loading texture \"%s\"\n", filepath);
		}
	}

	~TextureColorShader() {
		stbi_image_free(data);
	}

	void Shade(HitData& hitData, Material& out) {
		int x = hitData.u * (width - 1);
		int y = hitData.v * (height - 1);

		//clamp
		x = max(min(x, width-1), 0);
		y = max(min(y, height-1), 0);

		out.diffColor.x = (data[(y * width + x)*3 + 0] / 255.0f);
		out.diffColor.y = (data[(y * width + x)*3 + 1] / 255.0f);
		out.diffColor.z = (data[(y * width + x)*3 + 2] / 255.0f);
	}
};

class WoodColorShader : public ColorShader {
protected:
	Noise noise;
public:
	void Shade(HitData& hitData, Material& out) {
		//define a center
		float centerx = 0.2;
		float centery = 0.8;
		//distance to center
		float distx = hitData.u - centerx;
		float disty = hitData.v - centery;
		float dist = sqrt(distx*distx + disty*disty);
		//put into sin along with turbulence
		float turbulence = 6 * noise.octaveNoise(hitData.u, hitData.v, 0, 1, 3);
		//sqrt to decrease width of dark rings
		float sine = sqrt(abs(sin(dist * 10 * M_PI + turbulence)));
		//sine = (sine > 0.8) ? 1 : sine;

		//add some overall grain
		sine += 0.5 * noise.octaveNoise(hitData.u, hitData.v, 5, 4, 9);

		//attenuate and combine colors
		Vector3 dark(0.45f, 0.32f, 0.22f);
		Vector3 light(1, 0.83, 0.61);

		out.diffColor = dark + (light * 0.7 - dark) * sine;
		out.shininess = 4.0f;
	}
};

class FunColorShader : public ColorShader {
protected:
	Noise noise;

	// Source: AMD - https://en.wikipedia.org/wiki/Smoothstep
	float smoothstep(float edge0, float edge1, float x)
	{
		// Scale, bias and saturate x to 0..1 range
		x = ((x - edge0) / (edge1 - edge0));
		x = min(x, 1.0f);
		x = max(x, 0.0f);
		// Evaluate polynomial
		return x*x*(3 - 2 * x);
	}
public:
	void Shade(HitData& hitData, Material& out) {
		//create tubulent sine wave for transparency
		float turbulence = 10.0f * noise.octaveNoise(hitData.u, hitData.v, 0, 2, 9);
		float sine = abs(sin(hitData.u*0.2 + hitData.v * 0.3 + turbulence));
		//use this wave as ktran
		out.ktran = sine * 0.4;

		//create tubulent sine wave for specColor
		turbulence = 40.0f * noise.octaveNoise(hitData.u, hitData.v, 1, 2, 9);
		sine = 1 -abs(sin(hitData.u*0.2 + hitData.v * 0.3 + turbulence));
		//use this wave as speccolor
		out.specColor = Vector3(sine, sine, sine);
		out.shininess = sine;
	}
};

class GlassColorShader : public ColorShader {
public:
	void Shade(HitData& hitData, Material& out) {
		out.ktran = 0.714286f;
		out.shininess = 0.787037f;
		out.specColor = Vector3(0.357143f, 0.357143f, 0.357143f);
		out.diffColor = Vector3(0.194609f, 0.192348f, 0.204082);
	}
};

class HoleIntersectionShader : public IntersectionShader {
protected:
	Noise noise;

	// Source: AMD - https://en.wikipedia.org/wiki/Smoothstep
	float smoothstep(float edge0, float edge1, float x)
	{
		// Scale, bias and saturate x to 0..1 range
		x = ((x - edge0) / (edge1 - edge0));
		x = min(x, 1.0f);
		x = max(x, 0.0f);
		// Evaluate polynomial
		return x*x*(3 - 2 * x);
	}

public:
	bool Shade(const HitData& hitData) {
		float holes = 1.0f * smoothstep(0.0f, 0.3f, noise.noise(hitData.u*20, hitData.v*20, 0));
		return holes > 0.5f;
	}
};

class RainbowColorShader : public ColorShader {
protected:
	Noise noise;

public:
	void Shade(HitData& hitData, Material& out) {
		out.ktran = 0.0f;
		out.shininess = 0.787037f;
		out.specColor = Vector3(0.5f, 0.5f, 0.5f);


		out.diffColor.x = abs(noise.noise(hitData.u*2, hitData.v*2, 0)) * 0.8f + 0.2f;
		out.diffColor.y = abs(noise.noise(hitData.u*2, hitData.v*2, 1)) * 0.8f + 0.2f;
		out.diffColor.z = abs(noise.noise(hitData.u*2, hitData.v*2, 2)) * 0.8f + 0.2f;
	}
};