#pragma once

#include <memory>
#include "common/common.hpp"
#include "hitables/sphere.hpp"
#include "hitables/plane.hpp"
#include "emitters/envmap.hpp"

class Scene
{
public:
    std::vector<std::shared_ptr<Object>> objects;
    std::shared_ptr<EnvironmentMap> envMap;

    void makeScene() {

        envMap = std::make_shared<EnvironmentMap>();

        //std::shared_ptr<LambertUniform> lambertUniformPtr = std::make_shared<LambertUniform>(Spectrum(0.1, 0.4, 0.9));
        //std::shared_ptr<LambertUniform> lambertUniformPtr2 = std::make_shared<LambertUniform>(Spectrum(0.9, 0.4, 0.1));

        //objects.emplace_back(std::make_unique<Sphere>(Vector3(-1.0, 0.0, 0.0), 1.0, lambertUniformPtr, Spectrum(10.0, 10.0, 10.0)));
        //objects.emplace_back(std::make_unique<Sphere>(Vector3(0.2, 0.0, 0.0), 1.0, lambertUniformPtr2));


        //objects.emplace_back(std::make_unique<Plane>(Point3(0.0, -0.7, 0.0), Vector3(0.0,1.0,0.0)));
        objects.emplace_back(std::make_unique<Sphere>(Point3(1e5+1,40.8,81.6)  , 1e5 , std::make_shared<LambertUniform>(Spectrum(.75,.25,.25))));
        objects.emplace_back(std::make_unique<Sphere>(Point3(-1e5+99,40.8,81.6), 1e5 , std::make_shared<LambertUniform>(Spectrum(.25,.25,.75))));
        objects.emplace_back(std::make_unique<Sphere>(Point3(50,40.8,1e5)      , 1e5 , std::make_shared<LambertUniform>(Spectrum(.75))));
        objects.emplace_back(std::make_unique<Sphere>(Point3(50,1e5,81.6)      , 1e5 , std::make_shared<LambertUniform>(Spectrum(.75))));
        objects.emplace_back(std::make_unique<Sphere>(Point3(50,-1e5+81.6,81.6), 1e5 , std::make_shared<LambertUniform>(Spectrum(.75))));
        objects.emplace_back(std::make_unique<Sphere>(Point3(27,16.5,47)       , 16.5, std::make_shared<LambertUniform>(Spectrum(.999))));
        objects.emplace_back(std::make_unique<Sphere>(Point3(73,16.5,78)       , 16.5, std::make_shared<LambertUniform>(Spectrum(.999))));
        objects.emplace_back(std::make_unique<Sphere>(Point3(50,681.6-.27,81.6), 600 , std::make_shared<LambertUniform>(Spectrum(0.0)), Spectrum(12)));

    }
};