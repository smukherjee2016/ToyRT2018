#pragma once

#include <memory>
#include "common/common.hpp"
#include "hitables/sphere.hpp"
#include "hitables/plane.hpp"
#include "emitters/envmap.hpp"

class Scene
{
public:
    std::vector<std::unique_ptr<Hitable>> hitables;
    std::vector<std::shared_ptr<Emitter>> emitters;
    std::shared_ptr<EnvironmentMap> envMap;

    void makeScene() {

        envMap = std::make_shared<EnvironmentMap>();
        emitters.emplace_back(envMap);

        hitables.emplace_back(std::make_unique<Sphere>(Vector3(0.0, 0.0, 0.0), 0.7));
        hitables.emplace_back(std::make_unique<Plane>(Point3(0.0, -0.7, 0.0), Vector3(0.0,1.0,0.0)));

    }
};