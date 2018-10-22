#pragma once

#include "integrator.hpp"

class PathTracingIntegrator : public Integrator {
public:
    void render(const PinholeCamera& pinholeCamera, Film& film, const Scene& scene, const int sampleCount, const int numBounces) const override {
#pragma omp parallel for schedule(dynamic, 1)
        for(int i = 0; i < film.screenHeight * film.screenWidth; i++) {

            int positionInFilm = i;
            int x = positionInFilm % film.screenWidth;
            int y = positionInFilm / film.screenWidth;
            //int positionInFilm = y * film.screenWidth + x;

            Spectrum pixelValue(0.0);
            for (int j = 0; j < sampleCount; j++) {

                Ray cameraRay = pinholeCamera.generateCameraRay(x, y, film);
                Ray prevRay = cameraRay;

                Spectrum L(0.0);
                Spectrum Throughput (1.0);
                for(int k = 1; k <= numBounces; k++) {
                    std::optional<HitBundle> hitBundle = traceRayReturnClosestHit(prevRay, scene);
                    if (hitBundle) {

                        HitBundle prevRayHitBundle = hitBundle.value();

                        //If hit the emitter, return its Le multiplied by throughput
                        if(prevRayHitBundle.closestObject->isEmitter()){
                            L = prevRayHitBundle.closestObject->Le(prevRay) * Throughput;
                            break;
                        }
                        else {
                            //
                            Vector3 outgoingDirection = prevRayHitBundle.closestObject->mat->sampleDirection(-prevRay.d,
                                                                                                               prevRayHitBundle.hitInfo.normal);
                            Spectrum brdf = prevRayHitBundle.closestObject->mat->brdf(outgoingDirection, -prevRay.d,
                                                                                        prevRayHitBundle.hitInfo.normal);
                            Float pdfBSDF_BSDFSampling = prevRayHitBundle.closestObject->mat->pdfW(outgoingDirection,
                                                                                                     -prevRay.d,
                                                                                                     prevRayHitBundle.hitInfo.normal);
                            if(pdfBSDF_BSDFSampling == 0.0)
                                break;

                            Ray nextRay(prevRayHitBundle.hitInfo.intersectionPoint, outgoingDirection);
                            Throughput *= (brdf * glm::dot(outgoingDirection, prevRayHitBundle.hitInfo.normal) / pdfBSDF_BSDFSampling);
                            prevRay = nextRay;

                        }

                    }
                    else {
                        //Did not hit any emitter so hit env map. Thus get contribution from envmap with losses at material hits
                        //TODO Stop using env map as a special emitter and merge into existing emitter implementation
                        if(glm::any(glm::equal(Throughput, Vector3(0.0f))) || glm::any(glm::isnan(Throughput))) break;
                        L = scene.envMap->Le(prevRay) * Throughput;
                        break;
                    }


                }
                pixelValue += L;

            }
            pixelValue /= (sampleCount);
            film.pixels.at(positionInFilm) = pixelValue;
        }
    }

};