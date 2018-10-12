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
            return Spectrum(0.0, 0.0, 1.0); //Blue env map by default
        }
        else {
            //TODO Implement envmap sampling
            return Spectrum(0.0, 0.0, 0.0);
        }
    }

    Point3 samplePointOnEmitter(const Vector3& wo, const Vector3& normal) const override {
        return Point3(0.0);
    }

    Float pdfEmitterA(const Point3 &point) const override {
        return 0.0;
    }

    Vector3 getNormalForEmitter(const Point3& point) const override {
        return Vector3(0.0);
    }
};