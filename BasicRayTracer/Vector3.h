#pragma once
#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
using std::max;
using std::min;

// 3-D Vector
class Vector3 {
public:
	// data
	float x, y, z;

	// Initialize to (0,0,0)
	Vector3();
	// Initialize to (x,y, 0)
	Vector3(float x, float y);
	// Initialize to (x,y,z)
	Vector3(float x, float y, float z);
	// Intialize to (v[0], v[1], v[2])
	Vector3(const float v[3]);

	// Random unit vector in hemisphere of normal
	static Vector3 SampleNormalOrientedHemisphere(const Vector3& normal);

	void CreateNormalSpace(Vector3& tangent, Vector3& bitangent) const;

	// Addition
	Vector3 operator+(float scalar) const;
	Vector3 operator+(const Vector3& other) const;
	Vector3& operator+=(const Vector3& other);

	// Subtraction
	Vector3 operator-(float scalar) const;
	Vector3 operator-(const Vector3& other) const;

	// Multiplication
	Vector3 operator*(float scalar) const;
	Vector3 operator*(const Vector3& other) const;

	// Division
	Vector3 operator/(float scalar) const;
	Vector3 operator/(const Vector3& other) const;

	// Cross Product
	Vector3 cross(const Vector3& other) const;

	// Dot Product
	float dot(const Vector3& other) const {
		return x * other.x + y * other.y + z * other.z;
	}

	// Reflect across normal
	Vector3 reflect(const Vector3& normal) const {
		return normal * (normal.dot(*this)) * 2 - *this;
	}

	// Refract across normal
	// Returns true if total interal reflection occurs
	// Returns false normally and puts refraction vector in refractDir
	bool refract(const Vector3& normal, float n, Vector3& refractDir) const {
		float cosi = this->dot(normal);
		float ni = 1;
		float nt = n;
		Vector3 norm = normal;
		//flip if backwards
		if (cosi < 0)
			cosi = -cosi;
		else {
			std::swap(ni, nt);
			norm = -norm;
		}
		float eta = ni / nt;
		float k = 1 - eta * eta * (1 - cosi * cosi);
		if (k < 0)
			return true;
		refractDir = (*this) * eta + norm * (eta * cosi - sqrtf(k));
		return false;
	}

	// Square Root
	Vector3 SquareRoot() {
		return Vector3(sqrt(x), sqrt(y), sqrt(z));
	}

	// Exponential
	Vector3 Exp() {
		return Vector3(exp(x), exp(y), exp(z));
	}

	// Luminance
	float Luminance() {
		return 0.2126f * x + 0.7152f * y + 0.0722 * z;
	}

	float MaxComponent() {
		return max(x, max(y, z));
	}

	float Get(int axis) {
		if (axis == 0)
			return x;
		if (axis == 1)
			return y;
		return z;
	}

	void Set(int axis, float value) {
		if (axis == 0)
			x = value;
		else if (axis == 1)
			y = value;
		else if (axis == 2)
			z = value;
	}

	Vector3 operator -() const {
		return Vector3(-x, -y, -z);
	}

	float length() const {
		return sqrt(x*x + y*y + z*z);
	}

	Vector3 normalize() const {
		return *this / length();
	}
};

