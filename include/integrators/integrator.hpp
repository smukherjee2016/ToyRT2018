#pragma once

#include "common/common.hpp"
#include "camera/pinholecamera.hpp"

class Integrator {
    virtual void render(const PinholeCamera& pinholeCamera, Film& film) const = 0;
};