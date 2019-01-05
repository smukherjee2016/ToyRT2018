#pragma once

#include <memory>
#include <unordered_map>
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
#include "film/film.hpp"
#include "camera/pinholecamera.hpp"

class Scene
{
public:
    std::shared_ptr<Film> film;
    std::shared_ptr<Camera> camera;
    std::shared_ptr<EnvironmentMap> envMap;
    std::vector<std::shared_ptr<Emitter>> emitters;
    std::vector<Float> cdfEmitters;
    std::vector<Float> pdfsEmitters;
    std::unordered_map<std::string, std::shared_ptr<Object>> objectsHashTable;
    std::unordered_map<std::string, std::shared_ptr<Emitter>> emittersHashTable;
    std::unordered_map<std::string, std::shared_ptr<Material>> materialsHashTable;


    void parseAndSetupScene(const std::string sceneFile) {

        /*
         * Parses the scene file and sets up everything in it.
         * Currently does very little to prevent ill-formed inputs and crashes. TODO: Implement stricter checks for validity
         */
		std::ifstream inJsonStream(sceneFile);
		json sceneJson;
		try {
			sceneJson = json::parse(inJsonStream);
		}
		catch (const std::exception &e) {
			std::cerr << e.what() << std::endl;
		}
        inJsonStream.close();

        //Construct film
        {
            auto filmJSON = sceneJson.at("film");
            int xRes = filmJSON.at("XRes").get<int>();
            int yRes = filmJSON.at("YRes").get<int>();
            Float distanceToFilm = filmJSON.at("distanceToFilm").get<Float>();
            int FOV_degrees = filmJSON.at("FOV").get<int>();
            bool isXFOV = filmJSON.at("isXFOV").get<bool>();

            Float FOV_radians = static_cast<Float>(FOV_degrees) * PI / 180.0;
            film = std::make_unique<Film>(FOV_radians, distanceToFilm, xRes, yRes, isXFOV);
        }

        //Construct camera
        {
            auto cameraJSON = sceneJson.at("camera");
            const std::string cameraTypeString = cameraJSON.at("cameraType");
            if(cameraTypeString.compare("pinhole") == 0) {
                Point3 origin = cameraJSON.at("origin").get<Point3>();
                Point3 lookAt = cameraJSON.at("lookAt").get<Point3>();
                Vector3 up = cameraJSON.at("up").get<Vector3>();
                camera = std::make_unique<PinholeCamera>(origin, lookAt, up);
            }
            else {
                std::cerr << "Unsupported camera type: " << cameraTypeString << std::endl;
            }
        }

        //Construct materials
        for(auto & materialJSON : sceneJson.at("materials")) {

            const std::string materialTypeString = materialJSON.at("type");


            if(materialTypeString.compare("LambertCosine") == 0) {
                Spectrum kD = materialJSON.at("Kd").get<Spectrum>();
                std::string materialID = materialJSON.at("id");
                std::shared_ptr<LambertCosine> lambertCosineMaterial = std::make_shared<LambertCosine>(kD);
                lambertCosineMaterial->id = materialID;

                materialsHashTable.insert({materialID, lambertCosineMaterial});
            }
            else if(materialTypeString.compare("LambertUniform") == 0) {

                Spectrum kD = materialJSON.at("Kd").get<Spectrum>();
                std::string materialID = materialJSON.at("id");
                std::shared_ptr<LambertUniform> lambertUniformMaterial = std::make_shared<LambertUniform>(kD);
                lambertUniformMaterial->id = materialID;

                materialsHashTable.insert({materialID, lambertUniformMaterial});
            }
            else if(materialTypeString.compare("Phong") == 0) {

                Spectrum kS = materialJSON.at("Ks").get<Spectrum>();
                std::string materialID = materialJSON.at("id");

                int specularExponent = materialJSON.at("specularExponent").get<int>();

                std::shared_ptr<Phong> phongMaterial = std::make_shared<Phong>(kS, specularExponent);
                phongMaterial->id = materialID;

                materialsHashTable.insert({materialID, phongMaterial});

            }
            else if(materialTypeString.compare("BlinnPhong") == 0) {

                Spectrum kS = materialJSON.at("Ks").get<Spectrum>();
                std::string materialID = materialJSON.at("id");

                int specularExponent = materialJSON.at("specularExponent").get<int>();

                std::shared_ptr<BlinnPhong> blinnPhongMaterial = std::make_shared<BlinnPhong>(kS, specularExponent);
                blinnPhongMaterial->id = materialID;

                materialsHashTable.insert({materialID, blinnPhongMaterial});

            }
            else {
                std::cerr << "Unsupported material: " << materialTypeString << std::endl;
            }


        }


        //Construct objects
        for(auto & objectJSON : sceneJson.at("objects")) {

            const std::string objectTypeString = objectJSON.at("type");

            if(objectTypeString.compare("sphere") == 0) {
                Point3 centerOfSphere = objectJSON.at("center").get<Point3>();
                Float radiusOfSphere = objectJSON.at("radius").get<Float>();
                std::string materialKey = objectJSON.at("material").get<std::string>();
                auto materialPtr = materialsHashTable.find(materialKey)->second;

                std::string objectID = objectJSON.at("id").get<std::string>();
                std::shared_ptr<Sphere> sphereObject =  std::make_shared<Sphere>(centerOfSphere, radiusOfSphere, materialPtr);
                sphereObject->id = objectID;

                if(objectJSON.find("associatedEmitter") != objectJSON.end()) { // The object is an emitter then
                    std::string associatedEmitterKey = objectJSON.at("associatedEmitter").get<std::string>();
                    sphereObject->associatedEmitterID = associatedEmitterKey;
                }

                objectsHashTable.insert({objectID, sphereObject});
            }
            else if(objectTypeString.compare("plane") == 0) {
                //TODO Add support for planes
            }
            else if(objectTypeString.compare("mesh") == 0) {
                //Triangle mesh
                std::string materialKey = objectJSON.at("material").get<std::string>();
                auto materialPtr = materialsHashTable.find(materialKey)->second;

                std::string objectID = objectJSON.at("id").get<std::string>();

                //Read in parameters for the triangle mesh


                std::shared_ptr<TriangleMesh> meshObject =  std::make_shared<TriangleMesh>();
                meshObject->id = objectID;

                if(objectJSON.find("associatedEmitter") != objectJSON.end()) { // The object is an emitter then
                    std::string associatedEmitterKey = objectJSON.at("associatedEmitter").get<std::string>();
                    meshObject->associatedEmitterID = associatedEmitterKey;
                }

                objectsHashTable.insert({objectID, meshObject});
            }
            else {
                std::cerr << "Unsupported object type: " << objectTypeString << std::endl;
            }

        }

        //Construct emitters
        for(auto & emitterJSON : sceneJson.at("emitters")) {

            const std::string emitterTypeString = emitterJSON.at("emitterType");

            if(emitterTypeString.compare("area") == 0) {
                Spectrum Le = emitterJSON.at("Le").get<Spectrum>();
                std::shared_ptr<AreaLight> emitter = std::make_shared<AreaLight>(Le);
                const std::string emitterID = emitterJSON.at("id");
                emitter->id = emitterID;

                emittersHashTable.insert({emitterID, emitter});
            }
        }

        //Link emitters and their objects
        for(auto & objectHashElement : objectsHashTable) {
            for(auto & emitterHashElement : emittersHashTable) {
                std::shared_ptr<Object> objectPointer = objectHashElement.second;
                std::shared_ptr<Emitter> emitterPointer = emitterHashElement.second;


                if(emitterPointer->id.compare(objectPointer->associatedEmitterID) == 0) // Found matching object and emitter
                {
                    //std::cout << "Found emitter with id: " << emitterPointer->id << " matching with object: " << objectPointer->id << std::endl;
                    //Link the pointers from both sides, object and emitter
                    objectPointer->emitter = emitterPointer;
                    emitterPointer->setAssociatedObject(objectPointer);
                }
            }
        }

        for(auto emitterHashElement : emittersHashTable) {
            emitters.emplace_back(emitterHashElement.second);
        }

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
        for(size_t i = 0; i < emitters.size(); i++) {
            pdfsEmitters.emplace_back(heuristicsEmitters.at(i) / totalHeuristicValue);
        }

        Float currentPdfSum = 0.0;
        for(size_t i = 0; i < emitters.size(); i++) {
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

        for(size_t i = 0; i < emitters.size(); i++) {

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
        for(size_t i = 0 ; i < emitters.size(); i ++) {
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

        for(auto & object: objectsHashTable) {

            std::optional<HitInfo> hitInfoOptional = object.second->checkIntersectionAndClosestHit(ray);

            if (hitInfoOptional) {
                currentHitBundle.hitInfo = hitInfoOptional.value();
                hitSomething = true;
                currentHitBundle.closestObject = object.second;

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