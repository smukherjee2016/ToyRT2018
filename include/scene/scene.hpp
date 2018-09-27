#pragma once

#include <memory>
#include "common/common.hpp"
#include "hitables/sphere.hpp"
#include "hitables/plane.hpp"

class Scene
{
public:
    std::vector<std::unique_ptr<Hitable>> hitables;

    void makeScene() {

        hitables.emplace_back(std::make_unique<Sphere>(Vector3(0.0, 0.0, -1.0), 0.45));
        hitables.emplace_back(std::make_unique<Plane>(Point3(0.0, 0.0, -4.0), Vector3(0.0,0.0,-1.0)));
    }
};