#pragma once

#include "integrators/integrator.hpp"

class PathTracingEmitterv2 : public Integrator {

public:
    void render(const PinholeCamera& pinholeCamera, Film& film, const Scene& scene, const int sampleCount, const int numBounces) const {

    }
};