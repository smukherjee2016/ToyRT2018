#include "integrator.hpp"

class ToyIntegrator : public Integrator {
public:
    void render(const PinholeCamera& pinholeCamera, Film& film, const Scene& scene, const int sampleCount) const override {
        for(int y = 0; y < film.screenHeight; y++) {
            for(int x = 0; x < film.screenWidth; x++) {
                int positionInFilm = y * film.screenWidth + x;

                Ray cameraRay = pinholeCamera.generateCameraRay(x, y, film);

                Spectrum pixelValue{};

                std::optional<HitBundle> hitBundle = traceRayReturnClosestHit(cameraRay, scene);

                if(hitBundle) {
                    HitBundle validHitBundle = *hitBundle;
                    for(auto i = 0; i < sampleCount; i++) {
                        Vector3 outgoingDirection  = validHitBundle.closestObject->mat->sampleDirection(-cameraRay.d, validHitBundle.hitInfo.normal);
                        Spectrum brdf = validHitBundle.closestObject->mat->brdf(outgoingDirection, -cameraRay.d, validHitBundle.hitInfo.normal);
                        Point3 hitPoint = cameraRay.o + validHitBundle.hitInfo.tIntersection * cameraRay.d;
                        Ray nextRay(hitPoint, outgoingDirection);
                        pixelValue = brdf;

                    }

                    //pixelValue = outgoingDirection;
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