#pragma once
#include "Scene.h"
#include <vector>
#include <fstream>
#define USE_MATH_DEFINES
#include <cmath>

// Which pixel to record all rays for debugging
// View the rays using RayVisualizer.exe
#define RECORD_I 1031
#define RECORD_J 556

#define NUM_SAMPLING_PATTERNS 64
struct SamplePoint {
	float x;
	float y;
};

class Renderer {
private:
	// The scene to sample from
	Scene* scene;

	// Number of samples to take per pixel
	int samplesPerPixel;
	// Precomputed sample patterns
	std::vector<std::vector<SamplePoint>> samplePatterns;

	// Some constant internal parameters
	// Maximum bounces before recursion terminates
	const int MAX_BOUNCES = 10;
	// Shininess (specColor) needed to spawn a reflection ray
	const float MIN_SHININESS = 0.01f;
	// Transparency (ktran) needed to spawn a refraction ray
	const float MIN_TRANSPARENCY = 0.01f;
	// How far to extrude/intrude spawned reflection and refraction rays along the normal
	const float PUSH_SPAWNED_RAYS = 0.0001f;


	// Recursive function to trace a ray from origin in the given direction
	void traceRay(const Vector3& origin, const Vector3& direction, Vector3& outputColor, int numBounces, std::vector<Object*> insideStack, bool record);

	// Lighting
	LightSource* pickLight(float& lightPdf);
	Vector3 getLightRadiance(const Vector3& direction, const LightSource* light, const HitData& hitData);
	
	// Subsurface scattering
	Vector3 getSubsurfaceRadiance(const Vector3& direction, const HitData& hitData);
	Vector3 getSubsurfaceSingleScatterRadiance(const Vector3& direction, const HitData& hitData);
	Vector3 getSubsurfaceDiffuseRadiance(const Vector3& direction, const HitData& hitData);

	void createSamplingPatterns();

	// Recording points for debugging
	std::ofstream recordSegmentFile, recordNormalFile;
	void recordRay(const Vector3& origin, const Vector3& hitPoint, const Vector3& hitNormal);

public:
	// Create a renderer for the given scene
	Renderer(Scene* scene, int samplesPerPixel = 1);

	// Samples the pixel i,j and outputs the final color
	void ColorPixel(int i, int j, Vector3& outColor);
};