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
#include "materials/transparentMaterial.hpp"
#include "path/path.hpp" //TODO Check inclusion of defn. of HitBundle in path.hpp
#include "emitters/arealight.hpp"
#include "accel/embreewrapper.hpp"
#include "hitables/triangle.hpp"

class Scene
{
public:
    std::vector<std::shared_ptr<Object>> objects;
    std::shared_ptr<EnvironmentMap> envMap;
    std::vector<std::shared_ptr<Emitter>> emitters;
    std::vector<Float> cdfEmitters;
    std::vector<Float> pdfsEmitters;


    void makeScene() {

        envMap = std::make_shared<EnvironmentMap>();

        //Cbox-ish, source: smallpt, modified to make flipping normals unnecessary
        objects.emplace_back(std::make_unique<Sphere>(Point3(1e5+99,40.8,81.6)  , 1e5 , std::make_shared<LambertCosine>(Spectrum(.25,.25,.75)))); // Right wall (after modification
        objects.emplace_back(std::make_unique<Sphere>(Point3(-1e5+1,40.8,81.6), 1e5 , std::make_shared<LambertCosine>(Spectrum(.75,.25,.25)))); //Left wall (after modification)
        objects.emplace_back(std::make_unique<Sphere>(Point3(50,40.8,-1e5)      , 1e5 , std::make_shared<LambertCosine>(Spectrum(.75)))); //Front wall (after modification)
        objects.emplace_back(std::make_unique<Sphere>(Point3(50,1e5+81.6,81.6)      , 1e5 , std::make_shared<LambertCosine>(Spectrum(.75)))); //Ceiling (after modification)
        objects.emplace_back(std::make_unique<Sphere>(Point3(50,-1e5,81.6), 1e5 , std::make_shared<Phong>(Spectrum(0.01, 0.5, 0.999), 100))); //Floor (after modification)
        objects.emplace_back(std::make_unique<Sphere>(Point3(60,52,81.6), 6 , std::make_shared<LambertCosine>(Spectrum(.75))));
        //objects.emplace_back(std::make_unique<Sphere>(Point3(50,-1e5,81.6), 1e5 , std::make_shared<LambertCosine>(Spectrum(0.75)))); //Floor (after modification)

        std::shared_ptr<AreaLight> smolEmitter = std::make_shared<AreaLight>(Spectrum(8.24));
        std::shared_ptr<AreaLight> bigEmitter = std::make_shared<AreaLight>(Spectrum(12.34));


        std::shared_ptr<Sphere> smolEmitterSphere = std::make_shared<Sphere>(Point3(27,16.5,47)       , 1.5, std::make_shared<LambertCosine>(Spectrum(0)), smolEmitter);
        std::shared_ptr<Sphere> bigEmitterSphere =  std::make_shared<Sphere>(Point3(73,16.5,78)       , 16.5, std::make_shared<LambertCosine>(Spectrum(0)), bigEmitter);
        objects.emplace_back(smolEmitterSphere); //Emitters should always have zero transmission
        objects.emplace_back(bigEmitterSphere);


        smolEmitter->setAssociatedObject(smolEmitterSphere); //TODO Do something about this fragile call...
        bigEmitter->setAssociatedObject(bigEmitterSphere);
        emitters.emplace_back(smolEmitter);
        emitters.emplace_back(bigEmitter);

        //Scene with only emitters
//        objects.emplace_back(std::make_unique<Sphere>(Point3(27,16.5,47)       , 1.5, std::make_shared<LambertCosine>(Spectrum(0)), Spectrum(8.24)));
//        objects.emplace_back(std::make_unique<Sphere>(Point3(73,16.5,78)       , 16.5, std::make_shared<LambertCosine>(Spectrum(0)), Spectrum(12.34)));
//        objects.emplace_back(std::make_unique<Sphere>(Point3(73,73.5,78)       , 16.5, std::make_shared<LambertCosine>(Spectrum(0)), Spectrum(12.34)));

        //Construct emitter CDF Table
        std::vector<Float> heuristicsEmitters;
        Float totalHeuristicValue = 0.0;
        for(auto & emitter : emitters) {
            heuristicsEmitters.emplace_back(emitter->heuristicEmitterSelection());
            totalHeuristicValue += emitter->heuristicEmitterSelection();
        }
        for(int i = 0; i < emitters.size(); i++) {
            pdfsEmitters.emplace_back(heuristicsEmitters.at(i) / totalHeuristicValue);
        }

        Float currentPdfSum = 0.0;
        for(int i = 0; i < emitters.size(); i++) {
            currentPdfSum += pdfsEmitters.at(i);
            cdfEmitters.emplace_back(currentPdfSum);
        }

        //Handle triangle meshes loading and starting up Embree



    }

    std::optional<std::shared_ptr<Emitter>> selectRandomEmitter() const {

        if(emitters.size() == 0)
            return std::nullopt;

        Float randomFloat = rng.generate1DUniform();
        int randomEmitterIndex = 0;

        for(int i = 0; i < emitters.size(); i++) {

            Float prevcdf = (i == 0) ? 0.0 : cdfEmitters.at(i - 1);
            Float thiscdf = cdfEmitters.at(i);
            if(randomFloat >= prevcdf && randomFloat <= thiscdf) {
                randomEmitterIndex = i;
                break;
            }
        }
        std::shared_ptr<Emitter> ret = emitters.at(randomEmitterIndex);
        return ret;

        //Select random light source uniformly
//        int randomEmitterIndex = rng.generateRandomInt(emitters.size() - 1);
//
//        std::shared_ptr<Emitter> ret = emitters.at(randomEmitterIndex);
//        return ret;
    }

    Float pdfSelectEmitter(std::shared_ptr<Emitter> emitter) const {
        for(int i = 0 ; i < emitters.size(); i ++) {
            auto candidateEmitter = emitters.at(i);
            if(emitter == candidateEmitter)
                return pdfsEmitters.at(i);
        }
    }

    std::optional<HitBundle> traceRayReturnClosestHit(const Ray &ray) {
        HitBundle closestHitBundle{};
        closestHitBundle.hitInfo.tIntersection = Infinity;

        HitBundle currentHitBundle{};
        bool hitSomething = false;

        for(auto & object: objects) {

            std::optional<HitInfo> hitInfoOptional = object->checkIntersectionAndClosestHit(ray);

            if (hitInfoOptional) {
                currentHitBundle.hitInfo = hitInfoOptional.value();
                hitSomething = true;
                currentHitBundle.closestObject = object;

                if (currentHitBundle.hitInfo.tIntersection < closestHitBundle.hitInfo.tIntersection) {
                    closestHitBundle = currentHitBundle;
                }
            }

        }

        if(hitSomething)
            return { closestHitBundle };

        return std::nullopt;

    }


};