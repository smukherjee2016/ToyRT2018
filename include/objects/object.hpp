#pragma once

#include "common/common.hpp"
#include "materials/material.hpp"
#include <optional>
#include <memory>


class Object {
public:
    //Object
    virtual std::optional<HitInfo> checkIntersectionAndClosestHit(const Ray& ray) const = 0;
    std::shared_ptr<Material> mat;

    //Emitter
    virtual Spectrum Le(const Ray& incomingRay) const = 0;
    virtual Point3 samplePointOnEmitter(const Vector3& wo, const Vector3& normal) const = 0;
    virtual Float pdfEmitter(const Vector3& wi, const Vector3& wo, const Vector3& normal) const = 0;

    //IsEmitter
    virtual bool isEmitter() const = 0;
};