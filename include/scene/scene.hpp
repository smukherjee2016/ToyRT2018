#pragma once

#include <memory>
#include "common/common.hpp"
#include "hitables/sphere.hpp"
#include "hitables/plane.hpp"
#include "emitters/envmap.hpp"
#include "materials/lambertUniform.hpp"
#include "materials/lambertCosine.hpp"
#include "materials/blinnphong.hpp"
#include "materials/phong.hpp"

class Scene
{
public:
    std::vector<std::shared_ptr<Object>> objects;
    std::shared_ptr<EnvironmentMap> envMap;
    std::vector<std::shared_ptr<Object>> emitters;

    void makeScene() {

        envMap = std::make_shared<EnvironmentMap>();

        //Cbox-ish, source: smallpt, modified to make flipping normals unnecessary
        objects.emplace_back(std::make_unique<Sphere>(Point3(1e5+99,40.8,81.6)  , 1e5 , std::make_shared<LambertCosine>(Spectrum(.25,.25,.75)))); // Right wall (after modification
        objects.emplace_back(std::make_unique<Sphere>(Point3(-1e5+1,40.8,81.6), 1e5 , std::make_shared<LambertCosine>(Spectrum(.75,.25,.25)))); //Left wall (after modification)
        objects.emplace_back(std::make_unique<Sphere>(Point3(50,40.8,-1e5)      , 1e5 , std::make_shared<LambertCosine>(Spectrum(.75)))); //Front wall (after modification)
        objects.emplace_back(std::make_unique<Sphere>(Point3(50,1e5+81.6,81.6)      , 1e5 , std::make_shared<LambertCosine>(Spectrum(.75)))); //Ceiling (after modification)
        //objects.emplace_back(std::make_unique<Sphere>(Point3(50,-1e5,81.6), 1e5 , std::make_shared<Phong>(Spectrum(0.01, 0.5, 0.999), 100))); //Floor (after modification)
        objects.emplace_back(std::make_unique<Sphere>(Point3(50,-1e5,81.6), 1e5 , std::make_shared<LambertCosine>(Spectrum(0.75)))); //Floor (after modification)
        objects.emplace_back(std::make_unique<Sphere>(Point3(27,16.5,47)       , 1.5, std::make_shared<LambertCosine>(Spectrum(.0001)), Spectrum(8000.24)));
        objects.emplace_back(std::make_unique<Sphere>(Point3(73,16.5,78)       , 16.5, std::make_shared<LambertCosine>(Spectrum(0.0001)), Spectrum(12000.34)));
        objects.emplace_back(std::make_unique<Sphere>(Point3(60,52,81.6), 6 , std::make_shared<LambertCosine>(Spectrum(.75))));


        //Scene with only emitters
//        objects.emplace_back(std::make_unique<Sphere>(Point3(27,16.5,47)       , 1.5, std::make_shared<LambertCosine>(Spectrum(0)), Spectrum(8.24)));
//        objects.emplace_back(std::make_unique<Sphere>(Point3(73,16.5,78)       , 16.5, std::make_shared<LambertCosine>(Spectrum(0)), Spectrum(12.34)));
//        objects.emplace_back(std::make_unique<Sphere>(Point3(73,73.5,78)       , 16.5, std::make_shared<LambertCosine>(Spectrum(0)), Spectrum(12.34)));


        //Create the emitter list
        for(auto& object : objects) {
                if(object->isEmitter())
                    emitters.emplace_back(object);
        }
    }

    std::optional<std::shared_ptr<Object>> selectRandomEmitter() const {

            if(emitters.size() == 0)
                    return std::nullopt;

            //Select random light source uniformly
            int randomEmitterIndex = rng.generateRandomInt(emitters.size() - 1);

            std::shared_ptr<Object> ret = emitters.at(randomEmitterIndex);
            return ret;
    }

    Float pdfSelectEmitter(std::shared_ptr<Object> emitter) const {
            return 1.0 / emitters.size();
    }

};