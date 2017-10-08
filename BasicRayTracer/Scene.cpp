#include "Scene.h"

Scene::Scene(const char* sceneFile, int width, int height, float focalLength, float lensRadius) {
	//load scene
	SceneIO* scene = readScene(sceneFile);
	if (scene == NULL) {
		printf("Could not load %s.\n", sceneFile);
	}

	//create camera
	Vector3 cameraPos(scene->camera->position);
	Vector3 cameraView(scene->camera->viewDirection);
	Vector3 cameraUp(scene->camera->orthoUp);
	this->camera = new ThinLensCamera(cameraPos, cameraView, focalLength, cameraUp, scene->camera->verticalFOV, lensRadius, width, height);

	//load scene lights
	loadLights(scene);

	//load objects
	loadObjects(scene);

	//clean up IO info (we have copied it over to our data structures already)
	deleteScene(scene);

	//create acceleration structure
#ifdef ACCELERATION
	Timer preprocess_timer;
	preprocess_timer.startTimer();
	kdtree = new KDTree(primitives);
	preprocess_timer.stopTimer();
	printf("Preprocess time: %.5lf secs\n", preprocess_timer.getTime());
#endif
}

Material* Scene::makeMaterial(const MaterialIO* materialIO) {
	Material* material = new Material();
	material->ambColor = materialIO->ambColor;
	material->diffColor = materialIO->diffColor;
	material->emissColor = materialIO->emissColor;
	material->specColor = materialIO->specColor;
	material->shininess = materialIO->shininess;
	material->ktran = materialIO->ktran;
	material->bssrdf = NULL;
	return material;
}

void Scene::loadLights(const SceneIO* scene) {
	//load lights
	LightIO* lightNode = scene->lights;
	while (lightNode != NULL) {
		//create LightSource based on type
		LightSource* light = NULL;
		if (lightNode->type == LightType::POINT_LIGHT) {
			light = new PointLightSource(lightNode->position);
		}
		else if (lightNode->type == LightType::DIRECTIONAL_LIGHT) {
			light = new DirectionalLightSource(lightNode->direction);
		}
		else {
			printf("Unsupported LightType ignored.\n");
		}

		if (light != NULL) {
			//set color
			light->color = lightNode->color;
			//add to list
			lights.push_back(light);
		}

		//go to next light
		
		lightNode = lightNode->next;
	}
}

void Scene::loadObjects(const SceneIO* scene) {
	ObjIO* objNode = scene->objects;
	while (objNode != NULL) {
		//create Object
		Object* object = new Object();
		if (objNode->type == ObjType::SPHERE_OBJ) {
			// Sphere
			loadSphere(objNode, object);
		}
		else if (objNode->type == ObjType::POLYSET_OBJ) {
			// Polygon set
			loadPolyset(objNode, object);
		}
		else {
			printf("Unsupported ObjType ignored.\n");
			delete object;
			object = NULL;
		}

		//if object was created
		if (object != NULL) {
			objects.push_back(object);
		}
		//next object in linked list
		objNode = objNode->next;
	}
}

void Scene::loadSphere(const ObjIO* objNode, Object* parent) {
	SphereIO* sphereData = (SphereIO*)objNode->data;
	//create sphere and assign to object
	Sphere* sphere = new Sphere(Vector3(sphereData->origin), sphereData->radius);
	sphere->parent = parent;
	parent->primitives.push_back(sphere);

	//load sphere material
	Material* sphereMaterial = makeMaterial(objNode->material);
	sphere->material = sphereMaterial;

	//parent->colorShader = new FunColorShader();
	//object->intersectionShader = new CheckerBoardIntersectionShader(6, 6);

	//keep track of all primitives
	primitives.push_back(sphere);
	//keep track of all materials
	materials.push_back(sphereMaterial);
}

void Scene::loadPolyset(const ObjIO* objNode, Object* parent) {
	PolySetIO* polysetData = (PolySetIO*)objNode->data;

	//parent->colorShader = new RainbowColorShader();
	//parent->intersectionShader = new HoleIntersectionShader();

	//load all materials for this object
	std::vector<Material*> objectMaterials;
	for (int i = 0; i < objNode->numMaterials; i++) {
		Material* material = makeMaterial(&objNode->material[i]);
		objectMaterials.push_back(material);
		//keep track of all materials
		materials.push_back(material);
	}

	//create geometry
	if (polysetData->type == PolySetType::POLYSET_TRI_MESH) {
		//to texture map, assume all triangles are paired with the next triangle to make a quad
		//alternate between texture coord patterns after each triangle
		bool textureMapTriangle = true;
		//get all polygons (only triangles currently supported)
		for (int i = 0; i < polysetData->numPolys; i++) {
			if (polysetData->poly[i].numVertices != 3)
				printf("Unsupported non-triangle polygon detected.\n");

			//get vertex positions
			Vector3 v0 = polysetData->poly[i].vert[0].pos;
			Vector3 v1 = polysetData->poly[i].vert[1].pos;
			Vector3 v2 = polysetData->poly[i].vert[2].pos;

			//get vertex normals
			Vector3 n0, n1, n2;
			if (polysetData->normType == NormType::PER_VERTEX_NORMAL) {
				// per vertex norms
				n0 = Vector3(polysetData->poly[i].vert[0].norm).normalize();
				n1 = Vector3(polysetData->poly[i].vert[1].norm).normalize();
				n2 = Vector3(polysetData->poly[i].vert[2].norm).normalize();
			}
			else {
				//calculate normal
				//assume vertices are in clockwise order
				n0 = n1 = n2 = -((v2 - v0).cross(v1 - v0)).normalize();
			}
			//create triangle
			Triangle* triangle = new Triangle(v0, v1, v2, n0, n1, n2);

			//set material
			if (polysetData->materialBinding == MaterialBinding::PER_VERTEX_MATERIAL) {
				//look up per-vertex material
				triangle->m[0] = objectMaterials[polysetData->poly[i].vert[0].materialIndex];
				triangle->m[1] = objectMaterials[polysetData->poly[i].vert[1].materialIndex];
				triangle->m[2] = objectMaterials[polysetData->poly[i].vert[2].materialIndex];
			}
			else {
				//copy per-object material
				triangle->m[0] = triangle->m[1] = triangle->m[2] = objectMaterials[0];
			}

			//set texture coords depending on which triangle in the quad we are at
			if (textureMapTriangle) {
				triangle->tex[0] = Vector3(0, 0);
				triangle->tex[1] = Vector3(1, 0);
				triangle->tex[2] = Vector3(1, 1);
			}
			else {
				triangle->tex[0] = Vector3(0, 0);
				triangle->tex[1] = Vector3(1, 1);
				triangle->tex[2] = Vector3(0, 1);
			}

			//assign to object
			triangle->parent = parent;
			parent->primitives.push_back(triangle);

			//keep track of all primitives
			primitives.push_back(triangle);
			//texture map for next triangle in the quad or new quad
			textureMapTriangle = !textureMapTriangle;
		}
	}
	else {
		printf("Unsupported PolysetType ignored.\n");
	}
}

void Scene::SetObjectShader(int index, ColorShader* color, IntersectionShader* intersect) {
	objects[index]->colorShader = color;
	objects[index]->intersectionShader = intersect;
}

void Scene::SetObjectBSSRDF(int index, BSSRDF* bssrdf) {
	for (int i = 0; i < objects[index]->primitives.size(); i++)
		objects[index]->primitives[i]->SetBSSRDF(bssrdf);
}

void Scene::RemoveObject(int index) {
	//remove all primitives associated with object
	for (int i = 0; i < objects[index]->primitives.size(); i++) {
		auto it = std::find(primitives.begin(), primitives.end(), objects[index]->primitives[i]);
		delete (*it);
		primitives.erase(it);
	}
	objects.erase(objects.begin() + index);

	//rebuild kdtree
	delete kdtree;
	kdtree = new KDTree(primitives);
}