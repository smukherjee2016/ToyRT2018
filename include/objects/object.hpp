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
    virtual Point3 samplePointOnEmitter() const = 0;
    virtual Float pdfEmitterA(const Point3 &point) const = 0;
    virtual Vector3 getNormalForEmitter(const Point3& point) const = 0;

    //IsEmitter
    virtual bool isEmitter() const = 0;
};