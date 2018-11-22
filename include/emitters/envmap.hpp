#pragma once

#include "objects/object.hpp"
#include "util/pfmutils.hpp"

class EnvironmentMap : public Emitter {

    bool isConstColorEnvMap;
    PFMInfo envMap;
public:

    EnvironmentMap(const std::string file = "") {
        if(file != "") {
           envMap = readPFM(file);
           isConstColorEnvMap = false;
        }
        else {
            isConstColorEnvMap = true;
        }
        emitterType = ENVMAP;
    }

    Vector3 getNormalForEmitter(const Point3 &point) const override {
        return Vector3();
    }

    Spectrum heuristicEmitterSelection() const override {
        return Spectrum();
    }

    void setAssociatedObject(std::shared_ptr<Object> _associatedObject) override {
    }

    Float pdfSelectPointOnEmitterA(const Point3 &point) const override {
        return 0;
    }

    Point3 samplePointOnEmitter(Sampler sampler) const override {
        return Point3(0.0);
    }

    Spectrum Le(const Ray& incomingRay) const override {
        if(isConstColorEnvMap) {
            return Spectrum(0.0, 0.0, 1.0); //Blue env map by default
        }
        else {
            //TODO Implement envmap sampling
            return Spectrum(0.0, 0.0, 0.0);
        }
    }


};