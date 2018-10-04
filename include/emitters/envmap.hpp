#pragma once

#include "objects/object.hpp"
#include "util/pfmutils.hpp"

class EnvironmentMap : public Object {

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
    }

    //IsEmitter
    bool isEmitter() const override {
        return true;
    }

    //Object
    std::optional<HitInfo> checkIntersectionAndClosestHit(const Ray& ray) const override {
        return std::nullopt;
    }

    Spectrum Le(const Ray& incomingRay) const override {
        if(isConstColorEnvMap) {
            return Spectrum(1.0, 1.0, 1.0); //White env map by default
        }
        else {
            //TODO Implement envmap sampling
            return Spectrum(0.0, 0.0, 0.0);
        }
    }
};