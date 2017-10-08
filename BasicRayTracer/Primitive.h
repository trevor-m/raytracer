#pragma once
#include "Material.h"
#include "BoundingBox.h"
#include "Vector3.h"
#include <vector>
#include <cfloat>

class Object;

// A basic primitive that can be rendered needs only:
// 1) intersection function 
// 2) a parent object (for refraction purposes)
// 3) a method to get bounds (for the acceleration structure)
class Primitive {
public:
	Object* parent;

	// Intersection function
	virtual bool intersects(const Vector3& origin, const Vector3& direction, HitData& hitData) = 0;

	// Bounding box
	virtual BoundingBox GetBounds() = 0;

	// Midpoint
	virtual Vector3 GetMidpoint() = 0;

	virtual void SetBSSRDF(BSSRDF* bssrdf) = 0;
};

// A sphere
class Sphere : public Primitive {
public:
	Material* material;
	Vector3 center;
	float radius;

	Sphere(Vector3 center, float radius) : Primitive() {
		this->center = center;
		this->radius = radius;
	}

	bool intersects(const Vector3& origin, const Vector3& direction, HitData& hitData);
	BoundingBox GetBounds();
	Vector3 GetMidpoint();
	void SetBSSRDF(BSSRDF* bssrdf) {
		this->material->bssrdf = bssrdf;
	}
};

// Triangle primitive
// this could be a sub-class of Object if you wanted to render just a single triangle
// but for now they will only be in meshes
class Triangle : public Primitive {
public:
	// Vertex positions
	Vector3 v[3];
	// Normals (per vertex)
	Vector3 n[3];
	// Materials (per vertex)
	Material* m[3];
	// Texture Coords (per vertex)
	Vector3 tex[3];

	Triangle(Vector3 v0, Vector3 v1, Vector3 v2, Vector3 n0, Vector3 n1, Vector3 n2) {
		v[0] = v0;
		v[1] = v1;
		v[2] = v2;
		n[0] = n0;
		n[1] = n1;
		n[2] = n2;
	}

	bool intersects(const Vector3& origin, const Vector3& direction, HitData& hitData);
	BoundingBox GetBounds();
	Vector3 GetMidpoint();

	// A Texture mapping scheme that uses the same U,V parameterization as a sphere
	void MapTextureCoords(Vector3 objectCenter) {
		for (int i = 0; i < 3; i++) {
			//get direction to center of object
			Vector3 dir = (v[i] - objectCenter).normalize();

			//calculate u & v
			tex[i].x = 1.0f - (0.5f + atan2(dir.z, dir.x) / (2.0f * M_PI));
			tex[i].y = 0.5f - asin(dir.y) / M_PI;
		}
	}

	void SetBSSRDF(BSSRDF* bssrdf) {
		this->m[0]->bssrdf = this->m[1]->bssrdf = this->m[2]->bssrdf = bssrdf;
	}
};
