#pragma once
#include "Shader.h"
#include "Primitive.h"
#include <vector>

// An object contains 1 or more primitives and is considered a solid entity for purposes of refraction
class Object {
public:
	// n in Snell's law
	float indexOfRefraction;

	// Primitives that belong to this object
	std::vector<Primitive*> primitives;

	// How to shade the object
	ColorShader* colorShader;
	IntersectionShader* intersectionShader;

	Object() {
		indexOfRefraction = 1.5f;
		colorShader = NULL;
		intersectionShader = NULL;
	}

	// Get center point of a triangle mesh
	Vector3 GetCenter() {
		Vector3 center;
		for (int i = 0; i < primitives.size(); i++) {
			Triangle* triangle = (Triangle*)primitives[i];

			for (int j = 0; j < 3; j++) {
				center += triangle->v[j] * (1.0f / (3.0f * primitives.size()));
			}
		}
		return center;
	}

	// An alternate texture mapping scheme for triangle meshes which uses the same mapping as a sphere
	void AlternateTextureMap() {
		//spherical texture map
		Vector3 center = GetCenter();
		for (int i = 0; i < primitives.size(); i++)
			((Triangle*)primitives[i])->MapTextureCoords(center);
	}
};