#pragma once
#include "Vector3.h"
#include "BSSRDF.h"

// Material properties of an object. Some objects may share the same material
class Material {
public:
	Vector3 diffColor;	/* Diffuse color				*/
	Vector3 ambColor;	/* Ambient color				*/
	Vector3 specColor;	/* Specular color 				*/
	Vector3 emissColor;	/* Emissive color				*/
	float shininess;	/* Shininess: 0.0 - 1.0.  Must be scaled (multiply by 128) before use as a Phong cosine exponent (q in our equation).  */
	float ktran;		/* Transparency: 0.0 - 1.0			*/
	BSSRDF* bssrdf;

	// Some useful operations for interpolating materials
	Material operator*(float scalar) const {
		Material result;
		result.diffColor = diffColor * scalar;
		result.ambColor = ambColor * scalar;
		result.specColor = specColor * scalar;
		result.emissColor = emissColor * scalar;
		result.shininess = shininess * scalar;
		result.ktran = ktran * scalar;
		result.bssrdf = bssrdf;
		return result;
	}

	Material operator+(const Material& other) const {
		Material result;
		result.diffColor = diffColor + other.diffColor;
		result.ambColor = ambColor + other.ambColor;
		result.specColor = specColor + other.specColor;
		result.emissColor = emissColor + other.emissColor;
		result.shininess = shininess + other.shininess;
		result.ktran = ktran + other.ktran;
		result.bssrdf = bssrdf;
		return result;
	}
};

// Information about a point of intersection
struct HitData {
	// Distance along the ray's direction from its origin
	float t;
	// position of the intersection = rayOrigin + rayDirection * t
	Vector3 position;
	// surface normal at the point of intersection
	Vector3 normal;
	// material (interpolated)
	// TODO: Some efficiency is lost by interpolating all hits regardless of whether it is the closest - this is necessary for shadow rays though
	Material material;
	// texture coordinates
	float u, v;
};