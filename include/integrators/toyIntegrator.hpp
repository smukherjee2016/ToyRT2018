#include "integrator.hpp"

class ToyIntegrator : public Integrator {
public:
    void render(const PinholeCamera& pinholeCamera, Film& film, const Scene& scene) const override {
        for(int y = 0; y < film.screenHeight; y++) {
            for(int x = 0; x < film.screenWidth; x++) {
                int positionInFilm = y * film.screenWidth + x;

                Ray cameraRay = pinholeCamera.generateCameraRay(x, y, film);
                HitInfo closestHit{};
                closestHit.tIntersection = Infinity;

                HitInfo currentHit{};
                std::shared_ptr<Hitable> closestObject{};
                Spectrum pixelValue{};
                bool hitSomething = false;
                //closestHit.tIntersection = Infinity;
                //pixelValue = Vector3(0.0, 0.1, 0.5);
                //pixelValue = cameraRay.d;
                for(auto & hitable: scene.hitables) {

                    if (hitable->didItHitSomething(cameraRay)) {
                        currentHit = hitable->returnClosestHit(cameraRay);
                        hitSomething = true;
                        closestObject = hitable;

                        if (currentHit.tIntersection < closestHit.tIntersection) {
                            closestHit = currentHit;
                            closestObject = hitable;
                        }
                    }

                }

                if(hitSomething) {
                    Vector3 outgoingDirection  = closestObject->mat->sampleDirection(cameraRay.d, closestHit.normal);
                    pixelValue = closestObject->mat->brdf(outgoingDirection, cameraRay.d, closestHit.normal);
                    pixelValue = outgoingDirection;
                    //pixelValue = Vector3(closestHit.tIntersection * glm::length(glm::normalize(cameraRay.d)));
                }
                else {
                    pixelValue = scene.envMap->Le(cameraRay);
                }

                film.pixels.at(positionInFilm) = pixelValue;
            }
        }
    }
};