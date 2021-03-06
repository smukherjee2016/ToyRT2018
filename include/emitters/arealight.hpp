#pragma once

#include "emitter.hpp"

class AreaLight : public Emitter {
public:
    Spectrum LeIntensity;

    Point3 samplePointOnEmitter(Sampler sampler) const override {
        return associatedObject->samplePointOnObject(Sampler());
    }

    Float pdfSelectPointOnEmitterA(const Point3&) const override {
        Float surfaceArea = associatedObject->surfaceArea();
        return 1.0 / surfaceArea;
    }

    Vector3 getNormalForEmitter(const Point3& point) const override {
        return associatedObject->getNormalAtPoint(point);
    }

    Spectrum Le(const Ray&) const override {
        return LeIntensity;
    }

    Float heuristicEmitterSelection() const override {
        Float surfaceArea = associatedObject->surfaceArea();
        return ( glm::length(LeIntensity) * surfaceArea);
    }


    AreaLight(Spectrum _Le, std::shared_ptr<Object> _associatedObject = nullptr) : LeIntensity(_Le) {
        associatedObject = _associatedObject;
        emitterType = AREA;
    }

    void setAssociatedObject(std::shared_ptr<Object> _associatedObject) override {
        associatedObject = _associatedObject;
    }

};