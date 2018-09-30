#pragma once

#include "material.hpp"

class LambertUniform : public Material {
public:
    Spectrum kD;

    LambertUniform(const Spectrum& _kD) {
        kD = _kD;
    }

    Vector3 sampleDirection(const Vector3& wi, const Vector3& normal) const {

    }

    Spectrum brdf(const Vector3& wi, const Vector3& w0, const Vector3& normal) const {

    }

    virtual Float pdf(const Vector3& wi, const Vector3& w0, const Vector3& normal) const {

    }

};