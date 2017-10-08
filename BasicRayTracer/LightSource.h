#pragma once
#include "Vector3.h"
#include <cfloat>

// A light source. Must be able to provide a direction to trace shadow rays and a distance
class LightSource {
public:
	Vector3 color;

	virtual void getDirection(const Vector3& position, Vector3& lightDirection) const = 0;
	virtual float getDistance(const Vector3& position) const = 0;
	virtual float getAttenuation(float distance) const = 0;
};

// A point light source - has a position
class PointLightSource : public LightSource {
public:
	Vector3 position;

	PointLightSource(const Vector3& position) {
		this->position = position;
	}

	void getDirection(const Vector3& position, Vector3& lightDirection) const {
		lightDirection = (this->position - position).normalize();
	}

	float getDistance(const Vector3& position) const {
		return (this->position - position).length();
	}

	float getAttenuation(float distance) const {
		return min(1.0f, 1.0f / (0.25f + 0.1f * distance + 0.01f * distance * distance));
	}
};

// A directional light source - has a direction
class DirectionalLightSource : public LightSource {
public:
	Vector3 direction;

	DirectionalLightSource(const Vector3& direction) {
		this->direction = direction;
	}

	void getDirection(const Vector3& position, Vector3& lightDirection) const {
		lightDirection = -direction.normalize();
	}

	float getDistance(const Vector3& position) const {
		return FLT_MAX;
	}

	float getAttenuation(float distance) const {
		return 1.0f;
	}
};