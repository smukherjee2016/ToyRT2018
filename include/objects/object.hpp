#pragma once

#include "common/common.hpp"
#include "materials/material.hpp"
#include <optional>
#include <memory>

class Emitter;

#include "emitters/emitter.hpp"

class Object {
public:
    //Object
    virtual std::optional<HitInfo> checkIntersectionAndClosestHit(const Ray& ray) const = 0;
    std::shared_ptr<Material> mat;
    std::shared_ptr<Emitter> emitter;
    std::string id;
    std::string associatedMaterialID;
    std::string associatedEmitterID;


    //Emitter
    virtual Point3 samplePointOnObject(Sampler sampler) const = 0;
    virtual Float pdfSelectPointOnObjectA(const Point3 &point) const = 0;
    virtual Vector3 getNormalAtPoint(const Point3 &point) const = 0;
    virtual Float surfaceArea() const = 0;

    //IsEmitter
    virtual bool isEmitter() const = 0;

protected:
    virtual ~Object(){}
};