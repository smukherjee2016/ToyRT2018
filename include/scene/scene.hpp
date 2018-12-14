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

    bool parseAndSetupScene(const std::string sceneFile) {

        std::ifstream inJsonStream(sceneFile);
        json sceneJson = json::parse(inJsonStream);
        inJsonStream.close();

        //Construct film

        //Construct camera

        //Construct materials
        for(auto & material : sceneJson.at("materials")) {


        }


        //Construct objects
        for(auto & object : sceneJson.at("objects")) {
           // std::cout << object << std::endl;
        }

        //Construct emitters
        for(auto & emitterJSON : sceneJson.at("emitters")) {
            // map TaskState values to JSON as strings


            std::string emitterTypeString = emitterJSON.at("emitterType");

            if(emitterTypeString.compare("area") == 0) {
                Spectrum Le = emitterJSON.at("Le").get<Spectrum>();
            }
        }


        return true;
    }


    void makeScene(const std::string sceneFile) {

        envMap = std::make_shared<EnvironmentMap>();

        //Load and parse scene file
        parseAndSetupScene(sceneFile);

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
        return 0.0;
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