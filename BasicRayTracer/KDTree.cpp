#include "KDTree.h"

KDTree::KDTree(const std::vector<Primitive*>& primitives) {
	//recursively build tree
	root = makeNode(primitives, 0);
}

KDTree::~KDTree() {
	//recursively destroy tree
	deleteNode(root);
}

 KDNode* KDTree::makeNode(const std::vector<Primitive*>& primitives, int depth) {
	//if no primitives left, we are done
	if (primitives.size() == 0) {
		return NULL;
	}

	//create node containing all of the triangles
	KDNode* node = new KDNode();
	nodePrimitives.push_back(primitives);
	node->primitivesIndex = nodePrimitives.size() - 1;

	//one primitive: create bounds for that primitive and finish
	if (primitives.size() == 1) {
		//get bounds of all primitives in this node
		node->bounds = primitives[0]->GetBounds();
		for (int i = 1; i < primitives.size(); i++)
			node->bounds.Expand(primitives[i]->GetBounds());
		node->left = NULL;
		node->right = NULL;
		return node;
	}

	//determine which axis to split on
	//TODO: could use axis with most variance
	int axis = depth % 3;

	//find midpoint along axis of all primitives in this node
	//this will be where the axis is split
	float mid = 0.0f;
	for (int i = 0; i < primitives.size(); i++) {
		mid += primitives[i]->GetMidpoint().Get(axis);
	}
	mid /= (float)primitives.size();

	//seprate the primitives that lie on left and right sides of midpoint on chosen axis
	std::vector<Primitive*> left;
	std::vector<Primitive*> right;
	for (int i = 0; i < primitives.size(); i++) {
		if (primitives[i]->GetMidpoint().Get(axis) >= mid)
			right.push_back(primitives[i]);
		else
			left.push_back(primitives[i]);
	}

	//was this split trivial
	if (left.size() == 0)
		left = right;
	if (right.size() == 0)
		right = left;

	//if more than STOP_PERCENT are in both sides, we can stop dividing
	int sharedCount = 0;
	for (int i = 0; i < left.size(); i++) {
		for (int j = 0; j < right.size(); j++) {
			if (left[i] == right[j])
				sharedCount++;
		}
	}

	//do we need to keep going?
	if (sharedCount < STOP_PERCENT * left.size() && sharedCount < STOP_PERCENT * right.size()) {
		//yes, create left and right nodes recursively
		node->left = makeNode(left, depth + 1);
		node->right = makeNode(right, depth + 1);

		//get overall bounds of all primitives in this node
		node->bounds = node->left->bounds;
		node->bounds.Expand(node->right->bounds);
	}
	else {
		//no, stop
		node->left = NULL;
		node->right = NULL;

		//get bounds of all primitives in this node
		node->bounds = primitives[0]->GetBounds();
		for (int i = 1; i < primitives.size(); i++)
			node->bounds.Expand(primitives[i]->GetBounds());
	}

	return node;
}

 bool KDTree::intersectsNode(KDNode* node, const Vector3& origin, const Vector3& direction, const Vector3& invDirection, HitData& hitData, Object** hitObject, float* tMax) {
	 //does ray intersect this node's bounds?
	 if (node->bounds.intersects(origin, invDirection)) {
		 //if this a leaf?
		 if (node->left == NULL && node->right == NULL) {
			 // check all primitives in leaf
			 bool hit = false;
			 //iterate over all primitives
			 for (int i = 0; i < nodePrimitives[node->primitivesIndex].size(); i++) {
				 HitData thisHitData;
				 //hit and closer than all the rest?
				 if (nodePrimitives[node->primitivesIndex][i]->intersects(origin, direction, thisHitData) && thisHitData.t < *tMax) {
					 *tMax = thisHitData.t;
					 hit = true;
					 //put hit data in outputs
					 *hitObject = nodePrimitives[node->primitivesIndex][i]->parent;
					 hitData = thisHitData;
				 }
			 }
			 return hit;
		 }
		 else {
			 //not a leaf, keep traversing
			 bool hitLeft = false, hitRight = false;
			 if (node->left != NULL)
				 hitLeft = intersectsNode(node->left, origin, direction, invDirection, hitData, hitObject, tMax);
			 if (node->right != NULL)
				 hitRight = intersectsNode(node->right, origin, direction, invDirection, hitData, hitObject, tMax);
			 return (hitLeft || hitRight);
		 }
	 }
	 return false;
 }

 void KDTree::traceShadowNode(KDNode* node, const Vector3& origin, const Vector3& direction, const Vector3& invDirection, Vector3& shadowFactor, float maxDist) {
	 //does ray intersect this node's bounds?
	 if (node->bounds.intersects(origin, invDirection)) {
		 //if this a leaf?
		 if (node->left == NULL && node->right == NULL) {
			 for (int i = 0; i < nodePrimitives[node->primitivesIndex].size(); i++) {
				 HitData thisHitData;
				 if (nodePrimitives[node->primitivesIndex][i]->intersects(origin, direction, thisHitData) && thisHitData.t < maxDist && thisHitData.t >= MIN_SHADOW_INTERSECT) {
					 //fully opaque? block all light
					 if (thisHitData.material.ktran < 0.01f) {
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
		 }
		 else {
			 //not a leaf, keep traversing
			 if (node->left != NULL)
				 traceShadowNode(node->left, origin, direction, invDirection, shadowFactor, maxDist);
			 if (node->right != NULL)
				 traceShadowNode(node->right, origin, direction, invDirection, shadowFactor, maxDist);
		 }
	 }
 }

 bool KDTree::GetClosestIntersection(const Vector3 & origin, const Vector3 & direction, HitData & hitData, Object** hitObject) {
	 //inverse the direction of the ray for faster bounds intersection test
	 Vector3 invDirection = Vector3(1.0f / direction.x, 1.0f / direction.y, 1.0f / direction.z);

	 //start with the closest intersection being very far
	 float tMax = FLT_MAX;

	 //recursively traverse tree to check for intersection
	 return intersectsNode(root, origin, direction, invDirection, hitData, hitObject, &tMax);
 }

 void KDTree::TraceShadowRay(const Vector3& origin, const Vector3& direction, Vector3& shadowFactor, float maxDist) {
	 //inverse the direction of the ray for faster bounds intersection test
	 Vector3 invDirection = Vector3(1.0f / direction.x, 1.0f / direction.y, 1.0f / direction.z);

	 //start by allowing all light
	 shadowFactor = Vector3(1, 1, 1);

	 //recursively traverse tree and apply all intersections
	 return traceShadowNode(root, origin, direction, invDirection, shadowFactor, maxDist);
 }

 void KDTree::deleteNode(KDNode* node) {
	 if (node == NULL)
		 return;

	 deleteNode(node->left);
	 deleteNode(node->right);
	 delete node;
 }