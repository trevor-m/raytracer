#pragma once
#include "Vector3.h"
#include <algorithm>
#include <vector>
using std::min;
using std::max;

// Axis-Aligned Bounding Box
class BoundingBox {
public:
	Vector3 minCorner, maxCorner;

	// Expand a bounding box to encompass the new bounding box as well
	void Expand(const BoundingBox& bounds) {
		minCorner.x = min(minCorner.x, bounds.minCorner.x);
		minCorner.y = min(minCorner.y, bounds.minCorner.y);
		minCorner.z = min(minCorner.z, bounds.minCorner.z);
		maxCorner.x = max(maxCorner.x, bounds.maxCorner.x);
		maxCorner.y = max(maxCorner.y, bounds.maxCorner.y);
		maxCorner.z = max(maxCorner.z, bounds.maxCorner.z);
	}

	// Does a ray intersect with this bounding box?
	bool intersects(const Vector3& origin, const Vector3& invDirection) {
		Vector3 T1 = (minCorner - origin) * invDirection;
		Vector3 T2 = (maxCorner - origin) * invDirection;

		float tmin = min(T1.x, T2.x);
		tmin = max(tmin, min(T1.y, T2.y));
		tmin = max(tmin, min(T1.z, T2.z));

		float tmax = max(T1.x, T2.x);
		tmax = min(tmax, max(T1.y, T2.y));
		tmax = min(tmax, max(T1.z, T2.z));

		return (tmax >= tmin && tmax >= 0);
	}

	// Get longest axis of the bounding box
	int LongestAxis() {
		Vector3 delta = maxCorner - minCorner;
		float maxLength = delta.x;
		int axis = 0;
		if (delta.y > maxLength) {
			maxLength = delta.y;
			axis = 1;
		}
		if (delta.z > maxLength) {
			maxLength = delta.z;
			axis = 2;
		}
		return axis;
	}

	Vector3 GetMidpoint() {
		return (maxCorner + minCorner) * 0.5f;
	}
};