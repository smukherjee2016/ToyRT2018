#pragma once

#include "common/common.hpp"
#include "camera/pinholecamera.hpp"
#include "scene/scene.hpp"

class Integrator {
    virtual void render(const PinholeCamera& pinholeCamera, Film& film, const Scene& scene) const = 0;
};