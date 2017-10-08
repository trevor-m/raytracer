#include "KDTree.h"

KDTree::KDTree(const std::vector<Primitive*>& primitives) {
	//recursively build tree
	root = makeNode(primitives, 0);
}

KDTree::~KDTree() {
	//recursively destroy tree
	deleteNode(root);
}

#define SCORE_THRESHOLD 10
#define RESOLUTION 0.1f

#define THRESHOLD 10

bool KDTree::chooseSplitPlane(const std::vector<Primitive*>& primitives, BoundingBox bounds, int& axis, float& value) {
	float bestScore, score;
	bestScore = score = INFINITY;
	//try each dimension to find best split
	int d = 0;
	while(score > SCORE_THRESHOLD && d < 3) {
		//start by choosing midpoint
		float choice = bounds.GetMidpoint().Get(d);

		//split
		std::vector<Primitive*> left;
		std::vector<Primitive*> right;
		for (int i = 0; i < primitives.size(); i++) {
			if (primitives[i]->GetMidpoint().Get(d) >= choice)
				right.push_back(primitives[i]);
			else
				left.push_back(primitives[i]);
		}
		//count shared
		int shared = 0;
		for (int i = 0; i < left.size(); i++) {
			for (int j = 0; j < right.size(); j++) {
				if (left[i] == right[j])
					shared++;
			}
		}

		//score of this choice
		score = abs((int)left.size() - (int)right.size()) + shared;

		//do we need to shift to the left or right?
		if (left.size() > right.size())
			bounds.maxCorner.Set(d, choice);
		else if (right.size() > left.size())
			bounds.minCorner.Set(d, choice);
		else {
			d++;
			continue;
		}
		if (score < bestScore) {
			bestScore = score;
			axis = d;
			value = choice;
		}


		
		if (abs(bounds.maxCorner.Get(d) - bounds.minCorner.Get(d)) < RESOLUTION || score == primitives.size())
			d++;
	}
	return (score == primitives.size());
}

KDNode* KDTree::makeLeaf(const std::vector<Primitive*>& primitives) {
	KDNode* node = new KDNode();
	nodePrimitives.push_back(primitives);
	node->primitivesIndex = nodePrimitives.size() - 1;
	//get bounds of all primitives in this node
	node->bounds = primitives[0]->GetBounds();
	for (int i = 1; i < primitives.size(); i++)
		node->bounds.Expand(primitives[i]->GetBounds());
	node->left = NULL;
	node->right = NULL;
	return node;
}

 KDNode* KDTree::makeNode(const std::vector<Primitive*>& primitives, int depth) {
	 //if no primitives left, we are done
	 if (primitives.size() == 0) {
		 return NULL;
	 }

	 //one primitive: create leaf
	 if (primitives.size() == 1) {
		 return makeLeaf(primitives);
	 }

	 //get bounds of all primitives in this node
	 BoundingBox bounds = primitives[0]->GetBounds();
	 for (int i = 1; i < primitives.size(); i++)
		 bounds.Expand(primitives[i]->GetBounds());

	
	 //determine which axis to split on
	 int axis;
	 float value;
	 bool divisible = chooseSplitPlane(primitives, bounds, axis, value);
	 if (!divisible)
		 return makeLeaf(primitives);

	 //create node containing all of the triangles
	 KDNode* node = new KDNode();
	 nodePrimitives.push_back(primitives);
	 node->primitivesIndex = nodePrimitives.size() - 1;
	 node->bounds = bounds;

	 //seprate the primitives that lie on left and right sides of midpoint on chosen axis
	 std::vector<Primitive*> left;
	 std::vector<Primitive*> right;
	 for (int i = 0; i < primitives.size(); i++) {
		 if (primitives[i]->GetMidpoint().Get(axis) >= value)
			 right.push_back(primitives[i]);
		 else
			 left.push_back(primitives[i]);
	}

	//create left and right nodes recursively
	if(left.size() > THRESHOLD)
		node->left = makeNode(left, depth + 1);
	if (node->left == NULL)
		node->left = makeLeaf(primitives);
	if (right.size() > THRESHOLD)
		node->right = makeNode(right, depth + 1);
	if (node->right == NULL)
		node->right = makeLeaf(primitives);

	
	
	//node->right = makeNode(right, depth + 1);

	//get bounds for this node
	//->bounds = node->left->bounds;
	//node->bounds.Expand(node->right->bounds);

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