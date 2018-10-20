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

            Spectrum pixelValue{};
            for (int j = 0; j < sampleCount; j++) {

                Ray cameraRay = pinholeCamera.generateCameraRay(x, y, film);
                Ray prevRay = cameraRay;

                Float throughput = 1.0;
                for(int k = 0; k < numBounces; k++) {
                    std::optional<HitBundle> hitBundle = traceRayReturnClosestHit(prevRay, scene);
                    if (hitBundle) {

                        HitBundle prevRayHitBundle = hitBundle.value();

                        //If hit the emitter, return its Le
                        if(prevRayHitBundle.closestObject->isEmitter())
                            pixelValue += prevRayHitBundle.closestObject->Le(cameraRay);
                        else {
                            //BSDF sampling
                            Vector3 outgoingDirection = prevRayHitBundle.closestObject->mat->sampleDirection(-cameraRay.d,
                                                                                                               prevRayHitBundle.hitInfo.normal);
                            Spectrum brdf = prevRayHitBundle.closestObject->mat->brdf(outgoingDirection, -cameraRay.d,
                                                                                        prevRayHitBundle.hitInfo.normal);
                            Float pdfBSDF_BSDFSampling = prevRayHitBundle.closestObject->mat->pdfW(outgoingDirection,
                                                                                                     -cameraRay.d,
                                                                                                     prevRayHitBundle.hitInfo.normal);
                            Ray nextRay(prevRayHitBundle.hitInfo.intersectionPoint, outgoingDirection);
                            std::optional<HitBundle> nextRayHitBundle = traceRayReturnClosestHit(nextRay, scene);
                            if (nextRayHitBundle) {
                                HitBundle nextBundle = nextRayHitBundle.value();

                                //If hit a light source, return its Le
                                if(nextBundle.closestObject->isEmitter()) {
                                    Float pdfEmitter_BSDFSampling = nextBundle.closestObject->pdfEmitterA(
                                            nextBundle.hitInfo.intersectionPoint) * scene.pdfSelectEmitter(nextBundle.closestObject);

                                    //Convert the SA BSDF pdf into Area domain for MIS calculation
                                    Float squaredDistance = glm::length(nextBundle.hitInfo.intersectionPoint - prevRayHitBundle.hitInfo.intersectionPoint) *
                                                            glm::length(nextBundle.hitInfo.intersectionPoint - prevRayHitBundle.hitInfo.intersectionPoint);
                                    //Vector3 emitterNormal = nextBundle.closestObject->getNormalForEmitter(nextBundle.hitInfo.intersectionPoint);
                                    Float pdfBSDFA_BSDFSampling = pdfBSDF_BSDFSampling * glm::dot(outgoingDirection, prevRayHitBundle.hitInfo.normal) / squaredDistance;
                                    Float misweight = 1.0;//PowerHeuristic(1, pdfBSDFA_BSDFSampling, 1, pdfEmitter_BSDFSampling);

                                    pixelValue += nextBundle.closestObject->Le(nextRay) * brdf * glm::dot(outgoingDirection, prevRayHitBundle.hitInfo.normal) * misweight / pdfBSDF_BSDFSampling;
                                }
                                else {
                                    pixelValue += Vector3(0.0); //Since this is direct lighting, ignore bounce on other object
                                }


                        }
                        else {
                            //Did not hit any light source so zero contribution
                            //TODO Stop using env map as a special emitter and merge into existing emitter implementation
                            pixelValue += Vector3(0.0);
                        }

                        }

                    }
                    else { //Did not hit any object so hit environment map
                        pixelValue += scene.envMap->Le(cameraRay);
                        continue;
                    }
                }

            }
            pixelValue /= (sampleCount);
            film.pixels.at(positionInFilm) = pixelValue;
        }
    }

};