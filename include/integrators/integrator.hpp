#pragma once

#include "common/common.hpp"
#include "camera/pinholecamera.hpp"
#include "scene/scene.hpp"
#include "util/rng.hpp"
#include <optional>

class Integrator {
    virtual void render(const PinholeCamera& pinholeCamera, Film& film, const Scene& scene, const int sampleCount) const = 0;
};

struct HitBundle {
    HitInfo hitInfo;
    std::shared_ptr<Object> closestObject;
};

std::optional<HitBundle> traceRayReturnClosestHit(const Ray& ray, const Scene& scene) {
    HitBundle closestHitBundle{};
     closestHitBundle.hitInfo.tIntersection = Infinity;

    HitBundle currentHitBundle{};
    bool hitSomething = false;

    for(auto & object: scene.objects) {

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

struct EmitterBundle {
    std::shared_ptr<Object> emitter;
    Float pdfSelectEmitter;
};

inline std::optional<EmitterBundle> selectRandomEmitter(const Scene& scene) {
    std::vector<std::shared_ptr<Object>> emitters;
    for(auto& object : scene.objects) {
        if(object->isEmitter())
            emitters.emplace_back(object);
    }

    if(emitters.size() == 0)
        return std::nullopt;

    //Select random light source uniformly
    int randomEmitterIndex = rng.generateRandomInt(emitters.size() - 1);

    EmitterBundle ret;
    ret.emitter = emitters.at(randomEmitterIndex);
    ret.pdfSelectEmitter = 1.0 / static_cast<Float>(emitters.size());

    return ret;
}

inline Float PowerHeuristic(int nF, Float pdfF, int nG, Float pdfG) {
    Float f = nF * pdfF;
    Float g = nG * pdfG;
    return (f * f) / (f * f + g * g);
}