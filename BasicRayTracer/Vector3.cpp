#include "Vector3.h"

Vector3::Vector3() {
	x = y = z = 0.0f;
}

Vector3::Vector3(float x, float y) {
	this->x = x;
	this->y = y;
	this->z = 0.0f;
}

Vector3::Vector3(float x, float y, float z) {
	this->x = x;
	this->y = y;
	this->z = z;
}

Vector3::Vector3(const float v[3]) {
	x = v[0];
	y = v[1];
	z = v[2];
}

Vector3 Vector3::operator+(float scalar) const {
	return Vector3(x + scalar, y + scalar, z + scalar);
}

Vector3 Vector3::operator+(const Vector3& other) const {
	return Vector3(x + other.x, y + other.y, z + other.z);
}

Vector3& Vector3::operator+=(const Vector3& other) {
	this->x += other.x;
	this->y += other.y;
	this->z += other.z;
	return *this;
}

Vector3 Vector3::operator-(float scalar) const {
	return Vector3(x - scalar, y - scalar, z - scalar);
}

Vector3 Vector3::operator-(const Vector3& other) const {
	return Vector3(x - other.x, y - other.y, z - other.z);
}

Vector3 Vector3::operator*(float scalar) const {
	return Vector3(x * scalar, y * scalar, z * scalar);
}

Vector3 Vector3::operator/(float scalar) const {
	return Vector3(x / scalar, y / scalar, z / scalar);
}

Vector3 Vector3::operator/(const Vector3& other) const {
	return Vector3(x / other.x, y / other.y, z / other.z);
}

Vector3 Vector3::operator*(const Vector3& other) const {
	return Vector3(x * other.x, y * other.y, z * other.z);
}

Vector3 Vector3::cross(const Vector3& other) const {
	// x    y    z
	//v[x] v[y] v[z]
	//o[x] o[y] o[z]
	Vector3  result;
	result.x = y * other.z - z * other.y;
	result.y = -x * other.z + z * other.x;
	result.z = x * other.y - y * other.x;
	return result;
}

Vector3 Vector3::SampleNormalOrientedHemisphere(const Vector3& normal) {
	//random inputs
	float random1 = rand() / (float)RAND_MAX;
	float random12 = rand() / (float)RAND_MAX;

	//get uniform vector in hemisphere of (0,1,0)
	float sinTheta = sqrt(1.0f - random1 * random1);
	float phi = 2.0f * M_PI * random12;
	Vector3 sample = Vector3(cos(phi) * sinTheta, random1, sin(phi) * sinTheta);

	//orient along normal
	//compute tangent and bitangent
	Vector3 tangent, bitangent;
	normal.CreateNormalSpace(tangent, bitangent);

	//transform to world space from normal space
	return Vector3(sample.x * bitangent.x + sample.y * normal.x + sample.z * tangent.x,
		sample.x * bitangent.y + sample.y * normal.y + sample.z * tangent.y,
		sample.x * bitangent.z + sample.y * normal.z + sample.z * tangent.z);
}

void Vector3::CreateNormalSpace(Vector3& tangent, Vector3& bitangent) const {
	if (abs(x) > abs(y))
		tangent = Vector3(z, 0, -x) / sqrtf(x * x + z * z);
	else
		tangent = Vector3(0, -z, y) / sqrtf(y * y + z * z);
	bitangent = this->cross(tangent);
}