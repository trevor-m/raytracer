#pragma once
#include "scene_io.h"
#include "Object.h"
#include "Primitive.h"
#include "Material.h"
#include "Shader.h"
#include "LightSource.h"
#include "Camera.h"
#include "KDTree.h"
#include "Timer.h"
#include <vector>

#define FULLY_OPAQUE_THRESHOLD 0.01f
#define ACCELERATION

// Contains all of the information needed to render a scene
class Scene {
	// All objects in scene
	std::vector<Object*> objects;
	// All primitivies in scene
	std::vector<Primitive*> primitives;
	// All materials in scene - kept track of here for clean up purposes
	std::vector<Material*> materials;

	// Acceleration structure
	KDTree* kdtree;

	// Scene loading helper functions
	void loadLights(const SceneIO* scene);
	void loadObjects(const SceneIO* scene);
	// Creates a new Material from a MaterialIO
	Material* makeMaterial(const MaterialIO* materialIO);
	// Loads a sphere primitive
	void loadSphere(const ObjIO* objNode, Object* parent);
	// Loads a set of triangle primitives
	void loadPolyset(const ObjIO* objNode, Object* parent);

public:
	//All lights in the scene
	std::vector<LightSource*> lights;
	//Camera information to create rays
	ThinLensCamera* camera;

	// Loads a scene from sceneFile and sets up camera for image dimensions of [width x height]
	Scene(const char* sceneFile, int width, int height, float focalLength, float lensRadius);

	// Set object properties
	void SetObjectShader(int index, ColorShader* color, IntersectionShader* intersect);
	void SetObjectBSSRDF(int index, BSSRDF* bssrdf);
	void RemoveObject(int index);

	~Scene() {
		//clean up
		delete camera;
		//delete objects
		for (int i = 0; i < objects.size(); i++) {
			delete objects[i];
		}
		//delete primitives
		for (int i = 0; i < primitives.size(); i++) {
			delete primitives[i];
		}
		//delete materials
		for (int i = 0; i < materials.size(); i++) {
			delete materials[i];
		}
		//delete lights
		for (int i = 0; i < lights.size(); i++) {
			delete lights[i];
		}
#ifdef ACCELERATION
		//delete acceleration structure
		delete kdtree;
#endif
	}

	// Find the closest object the ray intersects with and outputs hit information
	bool GetClosestIntersection(const Vector3& origin, const Vector3& direction, HitData& hitData, Object** hitObject) const {
#ifdef ACCELERATION
		return kdtree->GetClosestIntersection(origin, direction, hitData, hitObject);
#else
		float closest = FLT_MAX;
		bool hit = false;
		//iterate over all primitives
		for (int i = 0; i < primitives.size(); i++) {
			HitData thisHitData;
			//hit and closer than all the rest?
			if (primitives[i]->intersects(origin, direction, thisHitData) && thisHitData.t < closest) {
				closest = thisHitData.t;
				hit = true;
				//put hit data in outputs
				*hitObject = primitives[i]->parent;
				hitData = thisHitData;
			}
		}
		return hit;
#endif
	}

	// Trace a shadow ray and output the color attenuation in shadowFactor
	void TraceShadowRay(const Vector3& origin, const Vector3& direction, Vector3& shadowFactor, float maxDist) const {
#ifdef ACCELERATION
		return kdtree->TraceShadowRay(origin, direction, shadowFactor, maxDist);
#else
		//start by allowing all light
		shadowFactor = Vector3(1, 1, 1);
		for (int i = 0; i < primitives.size(); i++) {
			HitData thisHitData;
			if (primitives[i]->intersects(origin, direction, thisHitData) && thisHitData.t < maxDist) {
				//fully opaque? block all light
				if (thisHitData.material.ktran < FULLY_OPAQUE_THRESHOLD) {
					shadowFactor = Vector3(0, 0, 0);
					return;
				}
				//normalize Cd
				float normFactor = thisHitData.material.diffColor.MaxComponent();
				//prevent div by 0
				Vector3 normalizedDiffuse = (normFactor > FLT_EPSILON) ? (thisHitData.material.diffColor / normFactor) : Vector3(1, 1, 1);
				//attenuate shadowFactor
				shadowFactor = shadowFactor * thisHitData.material.ktran * normalizedDiffuse;
			}
		}
#endif
	}
};