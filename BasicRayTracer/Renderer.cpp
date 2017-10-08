#include "Renderer.h"

Renderer::Renderer(Scene* scene, int samplesPerPixel) : samplesPerPixel(samplesPerPixel), scene(scene) {
	createSamplingPatterns();
}

void Renderer::createSamplingPatterns() {
	//create sampling patterns
	if (samplesPerPixel == 1) {
		//for 1 spp, always go through center of pixel
		SamplePoint p;
		p.x = 0.5f;
		p.y = 0.5f;
		samplePatterns.resize(1);
		samplePatterns[0].push_back(p);
	}
	else {
		//make sure samplesPerPixel is a power of 2
		if ((samplesPerPixel & (samplesPerPixel - 1)) != 0) {
			printf("Samples per pixel must be a power of 2.\n");
			exit(0);
		}

		int size = (int)sqrt(samplesPerPixel);
		//generate sample points
		samplePatterns.resize(NUM_SAMPLING_PATTERNS);
		for (int i = 0; i < NUM_SAMPLING_PATTERNS; i++) {
			for (int x = 0; x < size; x++) {
				for (int y = 0; y < size; y++) {
					//find bounds of this sample point
					float xMin = (float)x / size;
					float yMin = (float)y / size;
					float xMax = (float)(x + 1) / size;
					float yMax = (float)(y + 1) / size;

					//randomly choose values between bounds
					SamplePoint p;
					p.x = ((float)rand() / RAND_MAX) * (xMax - xMin) + xMin;
					p.y = ((float)rand() / RAND_MAX) * (yMax - yMin) + yMin;

					//add to array
					samplePatterns[i].push_back(p);
				}
			}
		}
	}
}

void Renderer::ColorPixel(int i, int j, Vector3& outputColor) {
	//record this ray?
	bool record = (j == RECORD_J && i == RECORD_I && samplesPerPixel == 1);
	//open file for writing
	if (record) {
		std::ofstream fout("recordScene.txt");
		fout << "../Scenes/test1.scene" << std::endl;
		fout.close();
		recordSegmentFile.open("recordRay.txt");
		recordNormalFile.open("recordNormal.txt");
	}

	Vector3 origin, direction;

	//decide which sample pattern to use
	int whichSamplePattern = (j * (int)scene->camera->imageWidth + i) % samplePatterns.size();

	//take samples
	std::vector<Vector3> sampleColors(samplesPerPixel);
	for (int n = 0; n < samplesPerPixel; n++) {
		//get position on image plane
		float x = i + samplePatterns[whichSamplePattern][n].x;
		float y = j + samplePatterns[whichSamplePattern][n].y;

		//get position on lens plane
		float u = rand() / (float)RAND_MAX;
		float v = rand() / (float)RAND_MAX;

		//get direction of this ray
		scene->camera->GetRay(x, y, u, v, origin, direction);

		//trace
		std::vector<Object*> insideStack;
		traceRay(origin, direction, sampleColors[n], 0, insideStack, record);
	}

	//average samples(box filter) and output
	outputColor = Vector3(0.0f, 0.0f, 0.0f);
	for (int n = 0; n < samplesPerPixel; n++) {
		outputColor += sampleColors[n];
	}
	outputColor = outputColor / (float)samplesPerPixel;
	

	//close file if recording
	if (record) {
		recordSegmentFile.close();
		recordNormalFile.close();
	}
}

void Renderer::traceRay(const Vector3& origin, const Vector3& direction, Vector3& outputColor, int numBounces, std::vector<Object*> insideStack, bool record) {
	if (numBounces > MAX_BOUNCES)
		return;

	HitData hitData;
	Object* hitObject = NULL;
	if (scene->GetClosestIntersection(origin, direction, hitData, &hitObject)) {
		//flip normal if inside object and hitting the other side of it
		if (!insideStack.empty() && std::find(insideStack.begin(), insideStack.end(), hitObject) != insideStack.end())
			hitData.normal = -hitData.normal;
		//always make normals face towards ray direction: can't rely on above for objects that use an intersection shader
		if (direction.dot(hitData.normal) > 0.0f) // TODO: && hitObject->intersectionShader != NULL
			hitData.normal = -hitData.normal;

		//record this segment for RayVisualizer
		if (record) {
			recordRay(origin, hitData.position, hitData.normal);
		}

		// Apply color shader
		if (hitObject->colorShader != NULL)
			hitObject->colorShader->Shade(hitData, hitData.material);

		//subsurface scattering
		if (hitData.material.bssrdf != NULL) {
			outputColor = getSubsurfaceRadiance(direction, hitData);
			return;
		}

		// Calculate lighting for this hit
		//ambient lighitng
		Vector3 radiance = hitData.material.ambColor * hitData.material.diffColor * (1.0f - hitData.material.ktran);

		//direct lighting: iterate over light sources
		for (int i = 0; i < scene->lights.size(); i++) {
			radiance += getLightRadiance(direction, scene->lights[i], hitData);
		}

		//create reflection ray
		Vector3 radianceReflection;
		if (hitData.material.specColor.MaxComponent() > MIN_SHININESS) {
			//push up starting point by epsilon
			Vector3 reflectOrigin = hitData.position + hitData.normal * PUSH_SPAWNED_RAYS;
			//calculate direction of ray
			Vector3 reflectDir = -direction.reflect(hitData.normal).normalize();

			//recurse
			traceRay(reflectOrigin, reflectDir, radianceReflection, numBounces + 1, insideStack, record);
		}

		//create refraction ray
		Vector3 radianceRefraction;
		bool totalInternalReflection = false;
		if (hitData.material.ktran > MIN_TRANSPARENCY) {
			//push away starting point to avoid self-intersection
			Vector3 refractOrigin;

			//calculate direction of ray
			float n1, n2;
			Vector3 refractDir;
			//have we entered this object before?
			auto insideObject = std::find(insideStack.begin(), insideStack.end(), hitObject);
			if (insideStack.empty() || insideObject == insideStack.end()) {
				//no, entering a new object
				refractOrigin = hitData.position - hitData.normal * PUSH_SPAWNED_RAYS;
				n1 = insideStack.empty() ? 1.0f : insideStack.back()->indexOfRefraction;
				insideStack.push_back(hitObject);
				n2 = hitObject->indexOfRefraction;
				totalInternalReflection = (-direction).refract(hitData.normal, n1 / n2, refractDir);
			}
			else {
				//yes, leaving object
				refractOrigin = hitData.position - hitData.normal * PUSH_SPAWNED_RAYS;
				n1 = hitObject->indexOfRefraction;
				insideStack.erase(insideObject);
				n2 = insideStack.empty() ? 1.0f : insideStack.back()->indexOfRefraction;
				totalInternalReflection = (-direction).refract(hitData.normal, n1 / n2, refractDir);
			}

			//recurse
			if (!totalInternalReflection)
				traceRay(refractOrigin, -refractDir, radianceRefraction, numBounces + 1, insideStack, record);
		}

		//apply rendering equation
		outputColor = radiance + radianceReflection * hitData.material.specColor + radianceRefraction * hitData.material.ktran;
	}
}

Vector3 Renderer::getLightRadiance(const Vector3& direction, const LightSource* light, const HitData& hitData) {
	//get direction to light
	Vector3 lightDir;
	light->getDirection(hitData.position, lightDir);
	float lightDist = light->getDistance(hitData.position);
	//get shadow factor
	Vector3 shadowFactor;
	//push up starting point by epsilon
	Vector3 shadowOrigin = hitData.position + hitData.normal * PUSH_SPAWNED_RAYS;
	scene->TraceShadowRay(shadowOrigin, lightDir, shadowFactor, lightDist);
	//attenuation
	float attenuation = light->getAttenuation(lightDist);
	//diffuse
	Vector3 radianceDiffuse = hitData.material.diffColor * max(lightDir.dot(hitData.normal), 0.0f) * (1.0f - hitData.material.ktran);
	//specular
	Vector3 reflectDir = lightDir.reflect(hitData.normal).normalize();
	Vector3 viewDir = (-direction).normalize();
	Vector3 radianceSpecular = hitData.material.specColor * pow(max(reflectDir.dot(viewDir), 0.0f), hitData.material.shininess * 128.0f);

	return (radianceDiffuse + radianceSpecular) * shadowFactor * light->color * attenuation;
}

void Renderer::recordRay(const Vector3& origin, const Vector3& hitPoint, const Vector3& hitNormal) {
	//start point
	recordSegmentFile << origin.x << " " << origin.y << " " << origin.z << std::endl;
	//end point
	recordSegmentFile << hitPoint.x << " " << hitPoint.y << " " << hitPoint.z << std::endl;

	//normals
	recordNormalFile << hitPoint.x << " " << hitPoint.y << " " << hitPoint.z << std::endl;
	recordNormalFile << (hitPoint.x + hitNormal.x) << " " << (hitPoint.y + hitNormal.y) << " " << (hitPoint.z + hitNormal.z) << std::endl;
}

Vector3 Renderer::getSubsurfaceRadiance(const Vector3& direction, const HitData& hitData) {
	return getSubsurfaceDiffuseRadiance(direction, hitData) + getSubsurfaceSingleScatterRadiance(direction, hitData);
}

Vector3 Renderer::getSubsurfaceSingleScatterRadiance(const Vector3& direction, const HitData& hitData) {
	BSSRDF* bssrdf = hitData.material.bssrdf;

	//refract 'outgoing' ray
	//assume we are always going from air to material
	float oneovereta = 1.0f / bssrdf->eta;
	Vector3 to;
	(-direction).refract(hitData.normal, oneovereta, to);
	to = (-to).normalize();
	//get fresnel transmittance factor
	float FtExitant = 1.0f - bssrdf->FresnelReflectance(abs(-direction.dot(hitData.normal)), bssrdf->eta);

	//angle of the outgoing (into our eye) light 
	float cosExitant = abs(direction.dot(hitData.normal));

	//take samples
	Vector3 singleScatter(0.0f, 0.0f, 0.0f);
	for (int i = 0; i < NUM_SUBSCATTER_SAMPLES; i++) {
		//sample with exponential falloff
		float depth = bssrdf->ImportanceSampleSingleScatter(rand() / (float)RAND_MAX);
		//create sample at this depth along refracted ray
		Vector3 samplePos = hitData.position + to * depth;

		//pick a light source and create a ray from the sample point in the light direction
		//TODO: pick other lights randomly
		float lightPdf;
		LightSource* light = pickLight(lightPdf);
		Vector3 lightDir;
		light->getDirection(samplePos, lightDir);

		//find closest intersection along this ray
		Object* hitObject;
		HitData intersection;
		if (scene->GetClosestIntersection(samplePos, lightDir, intersection, &hitObject)) {
			//if this point has a different BSSRDF, then it belongs to some other object and can be skipped
			if (intersection.material.bssrdf != bssrdf)
				continue;

			//sample light at this point
			//for now, pretend all surfaces are perfectly diffuse
			//not sure how to handle the view direction for the specular term
			intersection.material.specColor = Vector3(0, 0, 0);
			intersection.material.diffColor = Vector3(1, 1, 1);
			intersection.material.ktran = 0.0f;
			Vector3 lightRadiance = getLightRadiance(lightDir, light, intersection);

			//get distance from sample to intersection point on surface
			float si = (samplePos - intersection.position).length();
			float cosIncident = abs(lightDir.dot(intersection.normal));
			//use snells law to estimate true refracted distance through medium of the incoming ray
			float siPrime = bssrdf->TrueRefractedDistance(si, cosIncident, cosExitant);
			//Fresnel refractance of incoming ray
			float FtIncident = 1.0f - bssrdf->FresnelReflectance(cosIncident, oneovereta);

			//calculate phase (Henry-Greenstein phase function)
			float phase = bssrdf->Phase(lightDir.dot(direction));

			//combined extinction coefficient
			float G = abs(to.dot(intersection.normal)) / cosIncident;
			Vector3 sigmaTC = bssrdf->sigmaT + bssrdf->sigmaT * G;

			//product of two fresnel transmission terms
			float F = FtExitant * FtIncident;
			//outscattered radiance (broken up into parts
			Vector3 x1 = (bssrdf->sigmaS * F * phase) / sigmaTC;
			Vector3 x2 = (bssrdf->sigmaT * -siPrime).Exp();
			Vector3 x3 = (bssrdf->sigmaT * -depth).Exp();
			float pdf = lightPdf * bssrdf->SampleSingleScatterPDF(depth);
			singleScatter += (x1 * x2 * x3) * lightRadiance / pdf;
		}
	}
	singleScatter = singleScatter / NUM_SUBSCATTER_SAMPLES;

	return singleScatter;
}

Vector3 Renderer::getSubsurfaceDiffuseRadiance(const Vector3& direction, const HitData& hitData) {
	BSSRDF* bssrdf = hitData.material.bssrdf;

	//create basis for normal space for the intersected surface
	Vector3 tangent, bitangent;
	hitData.normal.CreateNormalSpace(tangent, bitangent);

	//get fresnel transmittance of exiting ray
	//assume we are always going from air to material
	float oneovereta = 1.0f / bssrdf->eta;
	float FtExitant = 1.0f - bssrdf->FresnelReflectance(abs(-direction.dot(hitData.normal)), bssrdf->eta);

	//take samples
	Vector3 diffuseScatter;
	for (int i = 0; i < NUM_SUBSCATTER_SAMPLES; i++) {
		//sample disk in normal space
		Vector3 samplePosNormalSpace = bssrdf->ImportanceSampleDiffusion(rand() / (float)RAND_MAX, rand() / (float)RAND_MAX);
		//transform to world space
		Vector3 samplePos = hitData.position + (tangent * samplePosNormalSpace.x + bitangent * samplePosNormalSpace.y);
		//float halfProbeLength = sqrt(Rmax)
		//samplePos += hitData.normal * 
		//assume this point is on the surface (only good for flat surfaces)
		HitData intersection;
		intersection.normal = hitData.normal;
		intersection.position = samplePos;

		//pick a light source and create a ray from the sample point in the light direction
		//TODO: pick other lights randomly
		float lightPdf;
		LightSource* light = pickLight(lightPdf);
		Vector3 lightDir;
		light->getDirection(intersection.position, lightDir);

		//sample light at this point
		//only doing diffuse
		intersection.material.specColor = Vector3(0, 0, 0);
		intersection.material.diffColor = Vector3(1, 1, 1);
		intersection.material.ktran = 0;
		Vector3 lightRadiance = getLightRadiance(lightDir, light, intersection);
		
		//get fresnel transmittance of incoming light at this point
		float cosIncident = abs(lightDir.dot(intersection.normal));
		float FtIncident = 1.0f - bssrdf->FresnelReflectance(cosIncident, oneovereta);

		//use dipole approximation to get Rd term, based on the distance to the sample point
		float r = (intersection.position - hitData.position).length();
		Vector3 Rd = bssrdf->DiffuseReflectance(r);

		//pdf of this sample is the pdf of choosing this light and of choosing this sample point
		float pdf = lightPdf * bssrdf->SampleDiffusionPDF(samplePosNormalSpace.x, samplePosNormalSpace.y);
		float F = FtExitant * FtIncident;
		diffuseScatter += (lightRadiance * Rd * F)/ (M_PI * pdf);
	}

	diffuseScatter = diffuseScatter / (float)NUM_SUBSCATTER_SAMPLES;
	return diffuseScatter;
}

LightSource* Renderer::pickLight(float& pdf) {
	//uniformly pick a random light
	int index = rand() % scene->lights.size();
	pdf = 1.0f / scene->lights.size();
	return scene->lights[index];
}