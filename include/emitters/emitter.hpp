#pragma once

#include "common/common.hpp"

class Emitter {
    virtual Spectrum Le(const Ray& incomingRay) const = 0;
};