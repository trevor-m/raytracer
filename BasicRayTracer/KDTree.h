#pragma once
#include "Primitive.h"
#include "BoundingBox.h"
#include <vector>

// To fix a glitch caused by incorrect normals on HW3/scene1:
// Requires that all shadow ray intersections be at least this far from the starting point
#define MIN_SHADOW_INTERSECT 0.0001f

class KDNode {
public:
	// Children
	KDNode* left;
	KDNode* right;
	// Bounds for this node
	BoundingBox bounds;
	// Index to all primitives contained within this node
	int primitivesIndex;
	//std::vector<Primitive*> primitives;

	KDNode() {
		left = right = NULL;
	}
};

class KDTree {
private:
	// First node of the tree
	KDNode* root;

	// Vector of vectors of primitives for each node
	std::vector<std::vector<Primitive*>> nodePrimitives;

	// When this percentage of primtives are contained in both children, stop creating new nodes
	const float STOP_PERCENT = 0.6f;

	// Recursive function to build tree
	KDNode* makeNode(const std::vector<Primitive*>& primitives, int depth);

	// Recursive function to delete tree
	void deleteNode(KDNode* node);

	// Recursive function for intersection
	bool intersectsNode(KDNode* node, const Vector3& origin, const Vector3& direction, const Vector3& invDirection, HitData& hitData, Object** hitObject, float* tMax);

	// Recursive function for shadow ray calculation
	void traceShadowNode(KDNode* node, const Vector3& origin, const Vector3& direction, const Vector3& invDirection, Vector3& shadowFactor, float maxDist);

public:
	// Build a KD-Tree with all of the given primitives
	KDTree(const std::vector <Primitive*>& primitives);
	~KDTree();

	// Intersection test
	bool GetClosestIntersection(const Vector3& origin, const Vector3& direction, HitData& hitData, Object** hitObject);

	// Trace a shadow ray
	void TraceShadowRay(const Vector3& origin, const Vector3& direction, Vector3& shadowFactor, float maxDist);
};