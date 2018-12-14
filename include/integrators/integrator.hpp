#pragma once

#include "common/common.hpp"
#include "camera/pinholecamera.hpp"
#include "scene/scene.hpp"
#include "util/sampler.hpp"
#include "path/pathsampler.hpp"
#include <optional>

class Integrator {
    virtual void render(std::shared_ptr<Camera> camera, std::shared_ptr<Film> film, Scene &scene, const int sampleCount,
                        const int numBounces) const = 0;
};

inline Float PowerHeuristic(Float pA, Float pB) {
    return (pA * pA) / (pA * pA + pB * pB);
}