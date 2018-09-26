#pragma once

#include <memory>
#include "common/common.hpp"
#include "hitables/sphere.hpp"

class Scene
{
public:
    std::vector<Sphere> spheres;

    void makeScene() {
        spheres.emplace_back(std::make_unique<Sphere>(Vector3(0.0, 0.0, -1.0), 1.0));
    }
};