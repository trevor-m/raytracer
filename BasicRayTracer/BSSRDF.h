#pragma once
#include "Vector3.h"

#define NUM_SUBSCATTER_SAMPLES 32

// Bidirectional
class BSSRDF {
private:
	// Input Parameters
	Vector3 sigmaA; // Absorption coefficient
	
	// Calculated parameters
	Vector3 sigmaTPrime; // Reduced extinction coefficient
	Vector3 alphaPrime; // Reduced albedo
	Vector3 D; // Diffusion constant
	float A; // Internal diffuse reflection coefficient
	Vector3 zr; // Distance beneath surface of positive dipole
	Vector3 zv; // Distance above surface of negative dipole

	float Rmax;

	// A rational approximation of the measured diffuse reflectance
	// Equation 5.27 from Donner's thesis
	float FresnelDiffuseReflectance(float eta) {
		if (eta > 1.0f)
			return (-1.4399f / (eta * eta)) + (0.7099f / eta) + 0.6681f + 0.0636f * eta;
		return -0.4399f + (0.7099f / eta) - (0.3319f / (eta * eta)) + (0.0636f / (eta*eta*eta));
	}

public:
	float g; // Anisotropy parameter (0 if isotropic)
	float eta; // Index of refraction
	Vector3 sigmaT;
	Vector3 sigmaS;
	Vector3 sigmaSPrime; // Scatter coefficient (prime) = sigmaPrime*(1-g)
	Vector3 sigmaTr; // Effective extinction coefficient

	BSSRDF(const Vector3& sigmaA, const Vector3& sigmaSPrime, float g, float eta) {
		this->sigmaA = sigmaA;
		this->sigmaSPrime = sigmaSPrime;
		this->g = g;
		this->eta = eta;

		//precompute sigmas
		this->sigmaS = sigmaSPrime / (1.0f - g);
		this->sigmaTPrime = sigmaA + sigmaSPrime;
		this->sigmaTr = (sigmaA * sigmaTPrime * 3.0f).SquareRoot();
		this->sigmaT = sigmaA + sigmaS;
		//alpha
		this->alphaPrime = sigmaSPrime / sigmaTPrime;

		//precompute A
		float Fdr = FresnelDiffuseReflectance(eta);
		this->A = (1.0f + Fdr) / (1.0f - Fdr);
		//precompute D
		this->D = Vector3(1.0f, 1.0f, 1.0f) / (sigmaTPrime * 3.0f);

		//compute mean free paths
		this->zr = Vector3(1.0f, 1.0f, 1.0f) / sigmaTPrime;
		this->zv = zr * (1.0f + 4.0f/3.0f * A); //zr + D * A * 4.0f; //

		//Rmax
		Rmax = sqrt(log(0.01f) / -sigmaTr.Luminance());
	}

	// Rd -  Diffuse Reflectance due to dipole sources
	// Equation 5.35 in http://www.cs.jhu.edu/~misha/Fall11/Donner.Thesis.pdf
	Vector3 DiffuseReflectance(float r) {
		//compute distances to dipole sources
		//positive dipole distance
		Vector3 dr = ((zr * zr) + r*r).SquareRoot();
		//negative dipole distance
		Vector3 dv = ((zv * zv) + r*r).SquareRoot();

		//positive contribution
		Vector3 sigmaTrDr = sigmaTr * dr;
		Vector3 pos = (zr * (sigmaTrDr + 1.0f) * (-sigmaTrDr).Exp()) / (dr * dr * dr);
		//negative contribution
		Vector3 sigmaTrDv = sigmaTr * dv;
		Vector3 neg = (zv * (sigmaTrDv + 1.0f) * (-sigmaTrDv).Exp()) / (dv * dv *dv);
		
		Vector3 Rd = (alphaPrime / (4.0f * M_PI)) * (pos + neg);

		//clamp to 0 to 1 range
		Rd.x = min(max(Rd.x, 0.0f), 1.0f);
		Rd.y = min(max(Rd.y, 0.0f), 1.0f);
		Rd.z = min(max(Rd.z, 0.0f), 1.0f);
		return Rd;
	}

	// Takes a uniform random from 0 to 1 and gets an exponential falloff
	// Used for depth along refracted ray
	float ImportanceSampleSingleScatter(float u) {
		return -log(u) / sigmaT.Luminance();
	}

	// The PDF for ImportanceSampleSingleScatter at value x
	float SampleSingleScatterPDF(float x) {
		return sigmaT.Luminance() * exp(-sigmaT.Luminance() * x);
	}

	// Sample a disk with exponential falloff
	// Within a radius Rmax
	Vector3 ImportanceSampleDiffusion(float u1, float u2) {
		float sigmaTrLuminance = sigmaTr.Luminance();
		float theta = 2 * M_PI * u1;
		//float r = sqrt(log(u2) / -sigmaTrLuminance);
		float r = sqrt(log(1.0f - u2 * (1.0f - exp(-sigmaTrLuminance * Rmax * Rmax))) / -sigmaTrLuminance);
		return Vector3(r*cos(theta), r*sin(theta), 0.0f);
	}

	// The PDF for a point chosen at x, y from ImportanceSampleDiffusion
	float SampleDiffusionPDF(float x, float y) {
		float sigmaTrLuminance = sigmaTr.Luminance();
		float pdf = 1.0f/M_PI * sigmaTrLuminance * exp(-sigmaTrLuminance * (x*x + y*y));
		return pdf / (1.0f - exp(-sigmaTrLuminance * Rmax * Rmax));
	}


	// Calculate Fresnel Reflectance for a dielectric
	// Using Schlick's Approximation
	float FresnelReflectance(float cosi, float eta) {
		cosi = max(cosi, 0.0f);
		float R0 = (eta - 1.0f) * (eta - 1.0f) / ((eta + 1.0f) * (eta + 1.0f));
		return R0 + (1.0f - cosi)* (1.0f - cosi) * (1.0f - cosi)*(1.0f - cosi)* (1.0f - cosi) * (1.0f - R0);
	}

	// Use Snell's law to estimate ratio of si (observed distance) to si' (the true refracted distance)
	// Equation 5.37 in Donner Thesis
	// According to Donner, this equation is incorrectly written in Jensen et al. 2001
	float TrueRefractedDistance(float si, float cosIncident, float cosExitant) {
		return si * cosIncident / sqrt(1 - (1.0f/(eta*eta))*(1.0f - cosExitant));
	}

	// Henyey-Greenstein phase function
	float Phase(float cosTheta) {
		return (1.0f / 4 * M_PI) * (1 - g*g) / ( pow(1 + 2 * g * cosTheta + g * g, 1.5f));
	}
};