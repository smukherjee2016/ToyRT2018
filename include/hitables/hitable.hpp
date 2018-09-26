#pragma once

#include "common/common.hpp"

class Hitable {
    virtual bool didItHitSomething(const Ray& ray) = 0;
    virtual HitInfo returnClosestHit(const Ray& ray) = 0;
};