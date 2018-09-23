#pragma once


#include "hitable.hpp"

class Sphere : public Hitable {
public:
    bool didItHitSomething() {
        return false;
    }

    HitInfo returnClosestHit() {
        glm::vec3 a = {0,0,0};
        return {a, a};
    }

private:

};