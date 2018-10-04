#include "integrator.hpp"

const int numBounces = 1;

class ToyIntegrator : public Integrator {
public:
    void render(const PinholeCamera& pinholeCamera, Film& film, const Scene& scene, const int sampleCount) const override {
#pragma omp parallel for schedule(dynamic, 1)
        for(int i = 0; i < film.screenHeight * film.screenWidth; i++) {

            int positionInFilm = i;
            int x = positionInFilm % film.screenWidth;
            int y = positionInFilm / film.screenWidth;
            //int positionInFilm = y * film.screenWidth + x;

            Spectrum pixelValue{};
            for (int j = 0; j < sampleCount; j++) {

                for(int k = 0; k < numBounces; k++) {
                    Ray cameraRay = pinholeCamera.generateCameraRay(x, y, film);

                    std::optional<HitBundle> hitBundle = traceRayReturnClosestHit(cameraRay, scene);

                    if (hitBundle) {
                        HitBundle validHitBundle = hitBundle.value();

                        //If hit the emitter, return its Le
                        if(validHitBundle.closestObject->isEmitter())
                            pixelValue += validHitBundle.closestObject->Le(cameraRay);
                        else {
                            Vector3 outgoingDirection = validHitBundle.closestObject->mat->sampleDirection(-cameraRay.d,
                                                                                                           validHitBundle.hitInfo.normal);
                            Spectrum brdf = validHitBundle.closestObject->mat->brdf(outgoingDirection, -cameraRay.d,
                                                                                    validHitBundle.hitInfo.normal);
                            Float pdf = validHitBundle.closestObject->mat->pdf(outgoingDirection, -cameraRay.d,
                                                                               validHitBundle.hitInfo.normal);
                            Point3 hitPoint = cameraRay.o + validHitBundle.hitInfo.tIntersection * cameraRay.d;
                            Ray nextRay(hitPoint, outgoingDirection);
                            std::optional<HitBundle> nextRayHitBundle = traceRayReturnClosestHit(nextRay, scene);
                            if (nextRayHitBundle) {
                                HitBundle nextBundle = nextRayHitBundle.value();

                                //If hit a light source, return its Le
                                if(nextBundle.closestObject->isEmitter()) {
                                    pixelValue += nextBundle.closestObject->Le(nextRay) * brdf * glm::dot(outgoingDirection, validHitBundle.hitInfo.normal) / pdf;
                                }
                                else {
                                    pixelValue += Vector3(0.0); //Since this is direct lighting, ignore bounce on other object
                                }


                            }
                            else {
                                pixelValue += scene.envMap->Le(nextRay) * brdf
                                              * glm::dot(outgoingDirection, validHitBundle.hitInfo.normal) / pdf;
                            }
                        }

                    } else { //Did not hit any object so hit environment map
                        pixelValue += scene.envMap->Le(cameraRay);
                    }
                }

            }
            pixelValue /= sampleCount;
            film.pixels.at(positionInFilm) = pixelValue;
        }
    }

};