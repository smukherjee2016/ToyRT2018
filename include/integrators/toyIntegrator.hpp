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
                            //Emitter Sampling
                            std::optional<EmitterBundle> emitterOptionalBundle = selectRandomEmitter(scene);
                            if(emitterOptionalBundle) {
                                EmitterBundle emitterBundle = emitterOptionalBundle.value();
                                Point3 pointOnLightSource = emitterBundle.emitter->samplePointOnEmitter(-cameraRay.d, validHitBundle.hitInfo.normal);
                                Vector3 outgoingDirection = glm::normalize(pointOnLightSource - validHitBundle.hitInfo.intersectionPoint);
                                Float pdfEmitterA_EmitterSampling = emitterBundle.emitter->pdfEmitterA(
                                        validHitBundle.hitInfo.intersectionPoint);

                                Spectrum brdf = validHitBundle.closestObject->mat->brdf(outgoingDirection, -cameraRay.d,
                                                                                        validHitBundle.hitInfo.normal);
                                Float pdfBSDF_EmitterSampling = validHitBundle.closestObject->mat->pdfW(
                                        outgoingDirection, -cameraRay.d,
                                        validHitBundle.hitInfo.normal);

                                Float tMax = glm::length(pointOnLightSource - validHitBundle.hitInfo.intersectionPoint) - epsilon;
                                Ray nextRay(validHitBundle.hitInfo.intersectionPoint, outgoingDirection,Infinity,epsilon,tMax);
                                //Ray nextRay(validHitBundle.hitInfo.intersectionPoint, outgoingDirection);

                                std::optional<HitBundle> nextRayHitBundle = traceRayReturnClosestHit(nextRay, scene);
                                if (!nextRayHitBundle) {
                                    //Unoccluded so we can reach light source
                                    Vector3 emitterNormal = emitterBundle.emitter->getNormalForEmitter(pointOnLightSource);

                                    Float squaredDistance = glm::length(pointOnLightSource - validHitBundle.hitInfo.intersectionPoint) * glm::length(pointOnLightSource - validHitBundle.hitInfo.intersectionPoint);
                                    Float geometryTerm =  glm::dot(outgoingDirection, validHitBundle.hitInfo.normal) * glm::dot(emitterNormal, -outgoingDirection)
                                            / squaredDistance;

                                    Float pdfBSDFA_EmitterSampling = pdfBSDF_EmitterSampling * glm::dot(outgoingDirection, validHitBundle.hitInfo.normal) / squaredDistance ; //Convert to area domain

                                    Float misWeight = 1.0;//PowerHeuristic(1, pdfEmitterA_EmitterSampling, 1, pdfBSDFA_EmitterSampling);
                                    pixelValue += emitterBundle.emitter->Le(nextRay) * brdf * geometryTerm * misWeight / pdfEmitterA_EmitterSampling;
                                    pixelValue /= emitterBundle.pdfSelectEmitter;

                                }
                                else {
                                    pixelValue += Vector3(0.0, 0.0, 0.0); //Light sample occluded so discard it
                                }

                            }
                            else { //No light sources in scene, so try to sample from envmap or return error
                                pixelValue += Vector3(0.0);
                            }

                            //BSDF sampling
                            Vector3 outgoingDirection = validHitBundle.closestObject->mat->sampleDirection(-cameraRay.d,
                                                                                                           validHitBundle.hitInfo.normal);
                            Spectrum brdf = validHitBundle.closestObject->mat->brdf(outgoingDirection, -cameraRay.d,
                                                                                    validHitBundle.hitInfo.normal);
                            Float pdfBSDF_BSDFSampling = validHitBundle.closestObject->mat->pdfW(outgoingDirection,
                                                                                                 -cameraRay.d,
                                                                                                 validHitBundle.hitInfo.normal);
                            Ray nextRay(validHitBundle.hitInfo.intersectionPoint, outgoingDirection);
                            std::optional<HitBundle> nextRayHitBundle = traceRayReturnClosestHit(nextRay, scene);
                            if (nextRayHitBundle) {
                                HitBundle nextBundle = nextRayHitBundle.value();

                                //If hit a light source, return its Le
                                if(nextBundle.closestObject->isEmitter()) {
                                    Float pdfEmitter_BSDFSampling = nextBundle.closestObject->pdfEmitterA(
                                            nextBundle.hitInfo.intersectionPoint);

                                    //Convert the SA BSDF pdf into Area domain for MIS calculation
                                    Float squaredDistance = glm::length(nextBundle.hitInfo.intersectionPoint - validHitBundle.hitInfo.intersectionPoint) *
                                                            glm::length(nextBundle.hitInfo.intersectionPoint - validHitBundle.hitInfo.intersectionPoint);
                                    //Vector3 emitterNormal = nextBundle.closestObject->getNormalForEmitter(nextBundle.hitInfo.intersectionPoint);
                                    Float pdfBSDFA_BSDFSampling = pdfBSDF_BSDFSampling * glm::dot(outgoingDirection, validHitBundle.hitInfo.normal) / squaredDistance;
                                    Float misweight = 0.0;//PowerHeuristic(1, pdfBSDFA_BSDFSampling, 1, pdfEmitter_BSDFSampling);

                                    pixelValue += nextBundle.closestObject->Le(nextRay) * brdf * glm::dot(outgoingDirection, validHitBundle.hitInfo.normal) * misweight / pdfBSDF_BSDFSampling;
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

                    } else { //Did not hit any object so hit environment map
                        pixelValue += scene.envMap->Le(cameraRay);
                    }
                }

            }
            pixelValue /= (sampleCount);
            film.pixels.at(positionInFilm) = pixelValue;
        }
    }

};