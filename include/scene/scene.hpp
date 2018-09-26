#pragma once

#include <memory>
#include "common/common.hpp"
#include "hitables/sphere.hpp"

class Scene
{
public:
    std::vector<Sphere> spheres;

    void makeScene() {
        spheres.emplace_back(Sphere(Vector3(0.0, 0.0, -1.0), 1.0));
    }
};