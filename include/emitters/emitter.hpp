#pragma once

class Object;
#include "objects/object.hpp"

class Emitter {
public:

    enum typeOfEmitter{
        AREA,
        ENVMAP,
        POINT
    };
    typeOfEmitter emitterType;
    std::shared_ptr<Object> associatedObject;

    //Emitter
    virtual Spectrum Le(const Ray& incomingRay) const = 0;
    virtual Point3 samplePointOnEmitter(Sampler sampler) const = 0;
    virtual Float pdfSelectPointOnEmitterA(const Point3 &point) const = 0;
    virtual Vector3 getNormalForEmitter(const Point3& point) const = 0;
    virtual Float heuristicEmitterSelection() const  = 0;
    virtual void setAssociatedObject(std::shared_ptr<Object> _associatedObject) = 0;



};