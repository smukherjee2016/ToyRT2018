#include "integrator.hpp"

const int numBounces = 1;

class ToyIntegrator : public Integrator {
public:
    void render(const PinholeCamera& pinholeCamera, Film& film, const Scene& scene, const int sampleCount) const override {
#pragma omp parallel for schedule(dynamic, 1)
        for(int i = 0; i < film.screenHeight * film.screenWidth; i++) {

            //int positionInFilm = y * film.screenWidth + x;
            int positionInFilm = i;
            int x = i % film.screenWidth;
            int y = film.screenHeight - i / film.screenWidth;
            Spectrum pixelValue{};
            for (int j = 0; j < sampleCount; j++) {

                for(int k = 0; k < numBounces; k++) {
                    Ray cameraRay = pinholeCamera.generateCameraRay(x, y, film);

                    std::optional<HitBundle> hitBundle = traceRayReturnClosestHit(cameraRay, scene);

                    if (hitBundle) {
                        HitBundle validHitBundle = hitBundle.value();
                        Vector3 outgoingDirection = validHitBundle.closestObject->mat->sampleDirection(-cameraRay.d,
                                                                                                       validHitBundle.hitInfo.normal);
                        Spectrum brdf = validHitBundle.closestObject->mat->brdf(outgoingDirection, -cameraRay.d,
                                                                                validHitBundle.hitInfo.normal);
                        Float pdf = validHitBundle.closestObject->mat->pdf(outgoingDirection, -cameraRay.d,
                                                                           validHitBundle.hitInfo.normal);
                        Point3 hitPoint = cameraRay.o + validHitBundle.hitInfo.tIntersection * cameraRay.d;
                        Float bias = (1.0 - 1e-4);
                        Ray nextRay(hitPoint, outgoingDirection * bias);
                        std::optional<HitBundle> nextRayHitBundle = traceRayReturnClosestHit(nextRay, scene);
                        if (nextRayHitBundle) {
                            HitBundle nextHitBundle = nextRayHitBundle.value();
                            if(nextHitBundle.hitInfo.tIntersection < 0.001) //Avoid self-intersection
                            //TODO Need to make this recursive when not doing directlighting
                                pixelValue += Vector3(0.0);
                            else
                                pixelValue += scene.envMap->Le(nextRay) * brdf
                                              * glm::dot(outgoingDirection, validHitBundle.hitInfo.normal) / pdf;
                        } else {
                            pixelValue += scene.envMap->Le(nextRay) * brdf
                                           * glm::dot(outgoingDirection, validHitBundle.hitInfo.normal) / pdf;
                        }
                        //pixelValue = outgoingDirection;
                        //pixelValue = Vector3(closestHit.tIntersection * glm::length(glm::normalize(cameraRay.d)));
                    } else {
                        pixelValue += scene.envMap->Le(cameraRay);
                    }
                }

            }
            pixelValue /= sampleCount;
            film.pixels.at(positionInFilm) = pixelValue;
        }
    }

};