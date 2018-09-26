#pragma once

#include "common/common.hpp"
#include "hitables/sphere.hpp"

class Scene
{
public:
    std::vector<Sphere> spheres;

    void makeScene();
};