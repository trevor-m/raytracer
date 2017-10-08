#define _USE_MATH_DEFINES
#include <cmath>
#include "Primitive.h"
#include "Object.h"

bool Sphere::intersects(const Vector3& origin, const Vector3& direction, HitData& hitData) {
	//set up quadratic
	Vector3 offset = origin - center;
	float a = direction.dot(direction);
	float b = direction.dot(offset) * 2.0f;
	float c = offset.dot(offset) - radius * radius;

	//get determinant
	float determinant = b*b - 4 * a*c;

	//one intersection
	if (determinant < FLT_EPSILON && determinant > -FLT_EPSILON) {
		float t = -b / (2.0f * a);
		if (t <= 0)
			return false;

		//set HitData
		hitData.t = t;
		hitData.position = origin + direction * t;
		hitData.normal = (hitData.position - center).normalize();
		hitData.material = *material;
		hitData.u = 0.5f + atan2(hitData.normal.z, hitData.normal.x) / (2.0f * M_PI);
		hitData.v = 0.5f - asin(hitData.normal.y) / M_PI;
		// check intersection shader
		if (parent->intersectionShader != NULL && !parent->intersectionShader->Shade(hitData))
			return false;
		return true;
	}
	//two intersections
	if (determinant > FLT_EPSILON) {
		float t1 = (-b + sqrt(determinant)) / (2.0f * a);
		float t2 = (-b - sqrt(determinant)) / (2.0f * a);
		//get closest greater than 0
		float t = 0;
		float further = max(t1, t2);
		if (t1 >= 0) {
			if (t2 >= 0)
				t = min(t1, t2);
			else
				t = t1;
		}
		else
			t = t2;

		if (t <= 0)
			return false;

		//set HitData
		hitData.t = t;
		hitData.position = origin + direction * t;
		hitData.normal = (hitData.position - center).normalize();
		hitData.material = *material;
		hitData.u = 1.0f - (0.5f + atan2(hitData.normal.z, hitData.normal.x) / (2.0f * M_PI));
		hitData.v = 0.5f - asin(hitData.normal.y) / M_PI;
		// check intersection shader
		if (parent->intersectionShader != NULL && !parent->intersectionShader->Shade(hitData)) {
			//if ray goes through this closer hit,
			//try further hit
			hitData.t = further;
			hitData.position = origin + direction * further;
			hitData.normal = (hitData.position - center).normalize();
			hitData.material = *material;
			hitData.u = 1.0f - (0.5f + atan2(hitData.normal.z, hitData.normal.x) / (2.0f * M_PI));
			hitData.v = 0.5f - asin(hitData.normal.y) / M_PI;

			if(!parent->intersectionShader->Shade(hitData))
				return false;
		}
		return true;
	}
	//else no roots determinant < 0
	return false;
}

// Moller-Trumbore intersection algorithm
// https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
bool Triangle::intersects(const Vector3& origin, const Vector3& direction, HitData& hitData) {
	//edges containing v0
	Vector3 edge1 = v[1] - v[0];
	Vector3 edge2 = v[2] - v[0];
	//begin calculating determinant
	Vector3 P = direction.cross(edge2);
	float det = edge1.dot(P);
	//det == 0, ray is in plane of triangle or is parallel to it
	if (det > -FLT_EPSILON && det < FLT_EPSILON)
		return false;
	float inv_det = 1.0f / det;

	//distance from v0 to ray origin
	Vector3 T = origin - v[0];

	//calc u and test bound
	float u = T.dot(P) * inv_det;
	//if intersection is outside of the triangle
	if (u < 0.0f || u > 1.0f)
		return false;

	//prepare to test v parameter
	Vector3 Q = T.cross(edge1);

	//calc v parameter and test bound
	float v = direction.dot(Q) * inv_det;
	//intersection is outside the triangle
	if (v < 0.0f || u + v > 1.0f)
		return false;

	float t = edge2.dot(Q) * inv_det;

	//intersection
	if (t > FLT_EPSILON) {
		//set HitData
		hitData.t = t;
		hitData.position = origin + direction * t;
		//interpolate normals, material, and texture coords using u & v
		hitData.normal = ((n[0] * (1.0f - (u + v))) + (n[1] * u) + (n[2] * v)).normalize();
		hitData.material = *m[0] * (1.0f - (u + v)) + (*m[1] * u) + (*m[2] * v);
		hitData.u = (tex[0].x * (1.0f - (u + v)) + (tex[1].x * u) + (tex[2].x * v));
		hitData.v = (tex[0].y * (1.0f - (u + v)) + (tex[1].y * u) + (tex[2].y * v));
		//check intersection shader
		if (parent->intersectionShader != NULL && !parent->intersectionShader->Shade(hitData))
			return false;
		return true;
	}
	return false;
}

BoundingBox Sphere::GetBounds() {
	BoundingBox bounds;
	//mins
	bounds.minCorner = center - radius;
	//maxes
	bounds.maxCorner = center + radius;
	return bounds;
}

BoundingBox Triangle::GetBounds() {
	BoundingBox bounds;
	//mins
	bounds.minCorner.x = min(v[0].x, min(v[1].x, v[2].x));
	bounds.minCorner.y = min(v[0].y, min(v[1].y, v[2].y));
	bounds.minCorner.z = min(v[0].z, min(v[1].z, v[2].z));
	//maxes
	bounds.maxCorner.x = max(v[0].x, max(v[1].x, v[2].x));
	bounds.maxCorner.y = max(v[0].y, max(v[1].y, v[2].y));
	bounds.maxCorner.z = max(v[0].z, max(v[1].z, v[2].z));

	return bounds;
}

Vector3 Sphere::GetMidpoint() {
	return center;
}

Vector3 Triangle::GetMidpoint() {
	return (v[0] + v[1] + v[2]) * (1.0f / 3.0f);
}