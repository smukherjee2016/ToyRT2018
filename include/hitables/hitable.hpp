#pragma once

#include "common/common.hpp"

class Hitable {
    virtual bool didItHitSomething() = 0;
    virtual HitInfo returnClosestHit() = 0;
};