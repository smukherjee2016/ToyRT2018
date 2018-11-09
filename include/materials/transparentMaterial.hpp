#pragma once

#include "material.hpp"

class TransparentMaterial : public Material {

    Vector3 sampleDirection(const Vector3& wo, const Vector3& normal) const override {
        return wo; //Return the same direction. Light passes through unmodified
    }
    Spectrum brdf(const Vector3& wi, const Vector3& wo, const Vector3& normal) const override {
        return Spectrum(1.0);
    }
    Float pdfW(const Vector3 &wi, const Vector3 &wo, const Vector3 &normal) const override {
        return 1.0;
    }
};