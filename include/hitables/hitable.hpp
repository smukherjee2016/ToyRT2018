#pragma once

#include "common/common.hpp"

class Hitable {
public:
    virtual bool didItHitSomething(const Ray& ray) const = 0;
    virtual HitInfo returnClosestHit(const Ray& ray) const = 0;
};