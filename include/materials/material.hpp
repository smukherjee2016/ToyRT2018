#pragma once

#include "common/common.hpp"

class Material {
public:
    virtual Vector3 sampleDirection(const Vector3 &wo, const Vector3 &normal, const Point2 pointInPSS) const = 0;
    virtual Spectrum brdf(const Vector3& wi, const Vector3& wo, const Vector3& normal) const = 0;
    virtual Float pdfW(const Vector3 &wi, const Vector3 &wo, const Vector3 &normal) const = 0;
};

inline bool areDirectionsSanitized(const Vector3 &wi, const Vector3 &wo, const Vector3 &normal) {
    if(glm::dot(wo, normal) < 0.0 || glm::dot(wi, normal) < 0.0)
        return false;

    return true;
}