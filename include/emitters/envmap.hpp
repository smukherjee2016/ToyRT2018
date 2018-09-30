#pragma once

#include "emitter.hpp"
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