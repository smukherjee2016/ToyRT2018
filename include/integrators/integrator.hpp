#pragma once

#include "common/common.hpp"
#include "camera/pinholecamera.hpp"
#include "scene/scene.hpp"
#include "util/sampler.hpp"
#include "path/pathsampler.hpp"
#include "tbb/tbb.h"
#include <optional>
#include <thread>

class Integrator {
    virtual void render(std::shared_ptr<Camera> camera, std::shared_ptr<Film> film, Scene &scene, const int sampleCount,
                        const int numBounces) const = 0;

protected:
    virtual ~Integrator(){}
};

inline Float PowerHeuristic(Float pA, Float pB) {
    return (pA * pA) / (pA * pA + pB * pB);
}