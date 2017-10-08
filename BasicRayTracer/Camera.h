#pragma once
#include "Vector3.h"

//A pinhole camera
class PinholeCamera {
	//camera
	Vector3 position; //E
	Vector3 viewDir; //V
	Vector3 upDir; //B
	Vector3 rightDir; //A
	
	//image
	Vector3 imageCenter; //M
	Vector3 imageVertical; //Y
	Vector3 imageHorizontal; //X

	//lens
	float focalDistance;
public:
	float imageWidth, imageHeight;

	PinholeCamera(Vector3 position, Vector3 viewDirection, float focalDistance, Vector3 orthoUp, float verticalFOV, int imageWidth, int imageHeight) {
		this->position = position;
		this->imageWidth = imageWidth;
		this->imageHeight = imageHeight;

		//normalize up and direction vector
		this->viewDir = viewDirection.normalize();
		orthoUp = orthoUp.normalize();

		//compute right vector (A)
		this->rightDir = viewDir.cross(orthoUp).normalize();
		//compute new up vector (B)
		this->upDir = rightDir.cross(viewDir).normalize();

		//compute center of image in world space (M)
		imageCenter = position + viewDir * focalDistance;

		//compute vertical vector (Y)
		imageVertical = upDir * focalDistance * tan(verticalFOV / 2.0f);

		//compute horizonal vector (X)
		float horizonalFOV = verticalFOV * ((float)imageWidth / imageHeight);
		imageHorizontal = rightDir * focalDistance * tan(horizonalFOV / 2.0f);
	}

	// Get a ray through a point on the image plane (pinhole camera)
	void GetRay(float x, float y, Vector3& origin, Vector3& direction) {
		//convert to [0,1] floating point coordinates
		float sx = x * (1.0f / imageWidth);
		float sy = y * (1.0f / imageHeight);

		//convert to world coordinates to get intersection with image plane
		//invert y axis so 0,0 is in bottom left corner
		Vector3 P = imageCenter + imageHorizontal * (2.0f * sx - 1.0f) + imageVertical * -(2.0f * sy - 1.0f);

		//direction goes from origin to point P in image plane
		direction = (P - position).normalize();
		//ray starts at camera position
		origin = position;
	}
};

// A camera with a thin lens for depth of field effects
class ThinLensCamera {
private:
	//camera
	Vector3 position; //E
	Vector3 viewDir; //V
	Vector3 upDir; //B
	Vector3 rightDir; //A

	//image
	Vector3 imageCenter; //M
	Vector3 imageVertical; //Y
	Vector3 imageHorizontal; //X

	//lens
	Vector3 focalPlanePosition; //position of focal plane - combined with viewDir defines plane
	float lensRadius;
	float focalDistance; //distance from lens to focal plane
	const float m = 1.0f; //distance between lens plane and image plane

	bool intersectsPlane(const Vector3& origin, const Vector3& direction, const Vector3& planeNormal, const Vector3& planePosition, float& t) {
		float dot = planeNormal.dot(direction);
		if (dot > FLT_EPSILON) {
			Vector3& offset = planePosition - origin;
			t = offset.dot(planeNormal) / dot;
			return (t >= 0);
		}
		return false;
	}
public:
	float imageWidth, imageHeight;

	ThinLensCamera(Vector3 position, Vector3 viewDirection, float focalDistance, Vector3 orthoUp, float verticalFOV, float lensRadius, int imageWidth, int imageHeight) {
		this->position = position;
		this->imageWidth = imageWidth;
		this->imageHeight = imageHeight;
		this->focalDistance = focalDistance;
		this->lensRadius = lensRadius;

		//normalize up and direction vector
		this->viewDir = viewDirection.normalize();
		orthoUp = orthoUp.normalize();

		//compute right vector (A)
		this->rightDir = viewDir.cross(orthoUp).normalize();
		//compute new up vector (B)
		this->upDir = rightDir.cross(viewDir).normalize();

		//compute center of image in world space (M)
		imageCenter = position - viewDir * m;

		//compute vertical vector (Y)
		imageVertical = upDir * m * tan(verticalFOV / 2.0f);

		//compute horizonal vector (X)
		float horizonalFOV = verticalFOV * ((float)imageWidth / imageHeight);
		imageHorizontal = rightDir * m * tan(horizonalFOV / 2.0f);

		//TODO: compute focal plane position
		focalPlanePosition = position + viewDir * focalDistance;

	}

	// Get a ray through a point on the lens and the image plane (thin lens DoF)
	void GetRay(float x, float y, float u, float v, Vector3& origin, Vector3& direction) {
		//convert to [0,1] floating point coordinates
		float sx = x * (1.0f / imageWidth);
		float sy = y * (1.0f / imageHeight);

		//convert to world coordinates to get point on image plane
		Vector3 P = imageCenter + imageHorizontal * -(2.0f * sx - 1.0f) + imageVertical * (2.0f * sy - 1.0f);

		//find point q on the focal plane where p focuses down to
		//a focused ray will go through the center of projection
		Vector3 focusDir = (position - P).normalize();
		//find where this ray intersects the focal plane
		float t;
		intersectsPlane(P, focusDir, viewDir, focalPlanePosition, t);
		Vector3 q = P + focusDir * t;

		//get starting point on lens
		origin = position + rightDir * (2.0f * u * lensRadius - lensRadius) + upDir * (2.0f * v * lensRadius - lensRadius);
		//get direction to focused point q
		direction = (q - origin).normalize();
	}
};