#pragma once

#include "common/common.hpp"

class Material {
public:
    virtual Vector3 sampleDirection(const Vector3& wo, const Vector3& normal) const = 0;
    virtual Spectrum brdf(const Vector3& wi, const Vector3& wo, const Vector3& normal) const = 0;
    virtual Float pdf(const Vector3& wi, const Vector3& wo, const Vector3& normal) const = 0;
};