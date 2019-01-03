#pragma once

#include "common/common.hpp"
#include "objects/object.hpp"


class TriangleMesh : public Object {
public:
    std::optional<HitInfo> checkIntersectionAndClosestHit(const Ray &ray) const override {
        return std::optional<HitInfo>();
    }

    Point3 samplePointOnObject(Sampler sampler) const override {
        return Point3();
    }

    Float pdfSelectPointOnObjectA(const Point3 &point) const override {
        return 0;
    }

    Vector3 getNormalAtPoint(const Point3 &point) const override {
        return Vector3();
    }

    Float surfaceArea() const override {
        return 0;
    }

    bool isEmitter() const override {
        return !(emitter == nullptr);
    }
};

class Triangle {

};