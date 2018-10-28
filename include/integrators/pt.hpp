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
                Float accumulatedBSDFpdfW = 1.0;
                Float accumulatedBSDFWAConversionFactor = 1.0;
                Float accumulatedGeometryTerms = 1.0;
                HitInfo lastBounceHitInfo{};
                Vector3 lastBounceSampledDirection(0.0);
                for(int k = 1; k <= numBounces; k++) {
                    std::optional<HitBundle> hitBundle = traceRayReturnClosestHit(prevRay, scene);
                    if (hitBundle) {

                        HitBundle prevRayHitBundle = hitBundle.value();

                        //If hit the emitter, return its Le multiplied by throughput
                        if(prevRayHitBundle.closestObject->isEmitter()){
#ifdef USE_LIGHT_SAMPLING //Must not double-count BSDF sampling->emitter
                            L += Vector3(0.0);
#else
                            if(k >= 2) {
                                //Geometry term from previous bounce
                                Float squaredDistance = glm::length(lastBounceHitInfo.intersectionPoint - prevRayHitBundle.hitInfo.intersectionPoint) * glm::length(lastBounceHitInfo.intersectionPoint - prevRayHitBundle.hitInfo.intersectionPoint);
                                Float geometryTerm =  std::max(0.0, glm::dot(lastBounceSampledDirection, lastBounceHitInfo.normal))  //Previous bounce dot product
                                                      * std::max(0.0, glm::dot(prevRayHitBundle.hitInfo.normal, -lastBounceSampledDirection)) //Current bounce dot product
                                                      / squaredDistance;
                                accumulatedGeometryTerms *= geometryTerm;


                                accumulatedBSDFWAConversionFactor *=  glm::dot(lastBounceSampledDirection, lastBounceHitInfo.normal) / squaredDistance;

                            }
                            L += prevRayHitBundle.closestObject->Le(prevRay) * Throughput * accumulatedGeometryTerms / (accumulatedBSDFpdfW * accumulatedBSDFWAConversionFactor);
#endif
                            break;
                        }
                        else {
#ifdef USE_LIGHT_SAMPLING
                            //Emitter Sampling
                            std::optional<std::shared_ptr<Object>> emitterOptionalBundle = scene.selectRandomEmitter();
                            if(emitterOptionalBundle) {
                                std::shared_ptr<Object> emitter = emitterOptionalBundle.value();
                                Point3 pointOnLightSource = emitter->samplePointOnEmitter(-prevRay.d,
                                                                                          prevRayHitBundle.hitInfo.normal);
                                Vector3 outgoingDirection = glm::normalize(
                                        pointOnLightSource - prevRayHitBundle.hitInfo.intersectionPoint);
                                Float pdfEmitterA_EmitterSampling = emitter->pdfEmitterA(
                                        prevRayHitBundle.hitInfo.intersectionPoint);

                                Spectrum brdf = prevRayHitBundle.closestObject->mat->brdf(outgoingDirection,
                                                                                            -prevRay.d,
                                                                                          prevRayHitBundle.hitInfo.normal);
                                Float pdfBSDF_EmitterSampling = prevRayHitBundle.closestObject->mat->pdfW(
                                        outgoingDirection, -prevRay.d,
                                        prevRayHitBundle.hitInfo.normal);

                                Float tMax =
                                        glm::length(pointOnLightSource - prevRayHitBundle.hitInfo.intersectionPoint) -
                                        epsilon;
                                Ray nextRay(prevRayHitBundle.hitInfo.intersectionPoint, outgoingDirection, Infinity,
                                            epsilon, tMax);

                                std::optional<HitBundle> nextRayHitBundle = traceRayReturnClosestHit(nextRay, scene);
                                if (!nextRayHitBundle) {
                                    //Unoccluded so we can reach light source
                                    Vector3 emitterNormal = emitter->getNormalForEmitter(pointOnLightSource);

                                    Float squaredDistance = glm::length(pointOnLightSource - prevRayHitBundle.hitInfo.intersectionPoint) * glm::length(pointOnLightSource - prevRayHitBundle.hitInfo.intersectionPoint);
                                    Float geometryTerm =  std::max(0.0, glm::dot(outgoingDirection, prevRayHitBundle.hitInfo.normal)) * std::max(0.0, glm::dot(emitterNormal, -outgoingDirection))
                                                          / squaredDistance;

                                    Float pdfBSDFA_EmitterSampling = pdfBSDF_EmitterSampling * glm::dot(outgoingDirection, prevRayHitBundle.hitInfo.normal) / squaredDistance ; //Convert to area domain

                                    Float compositeEmitterPdfA_EmitterSampling = pdfEmitterA_EmitterSampling * scene.pdfSelectEmitter(emitter);

                                    Float misWeight = 0.5;//PowerHeuristic(1, compositeEmitterPdfA_EmitterSampling, 1, pdfBSDFA_EmitterSampling);
                                    L += emitter->Le(nextRay) * Throughput * brdf * geometryTerm * misWeight / compositeEmitterPdfA_EmitterSampling;
                                    //pixelValue /= emitterBundle.pdfSelectEmitter;

                                }

                            }
#endif
                            //BSDF Sampling
                            Vector3 outgoingDirection = prevRayHitBundle.closestObject->mat->sampleDirection(-prevRay.d,
                                                                                                               prevRayHitBundle.hitInfo.normal);
                            Spectrum brdf = prevRayHitBundle.closestObject->mat->brdf(outgoingDirection, -prevRay.d,
                                                                                        prevRayHitBundle.hitInfo.normal);
                            Float pdfBSDF_BSDFSampling = prevRayHitBundle.closestObject->mat->pdfW(outgoingDirection,
                                                                                                     -prevRay.d,
                                                                                                     prevRayHitBundle.hitInfo.normal);
                            if(pdfBSDF_BSDFSampling == 0.0)
                                break;

                            if(k >= 2) {
                                //Area domain conversion and geometry term from previous bounce
                                Float squaredDistance = glm::length(lastBounceHitInfo.intersectionPoint - prevRayHitBundle.hitInfo.intersectionPoint) * glm::length(lastBounceHitInfo.intersectionPoint - prevRayHitBundle.hitInfo.intersectionPoint);
                                Float geometryTerm =  std::max(0.0, glm::dot(lastBounceSampledDirection, lastBounceHitInfo.normal))  //Previous bounce dot product
                                                      * std::max(0.0, glm::dot(prevRayHitBundle.hitInfo.normal, -lastBounceSampledDirection)) //Current bounce dot product
                                                      / squaredDistance;
                                accumulatedGeometryTerms *= geometryTerm;

                                accumulatedBSDFWAConversionFactor *=  glm::dot(lastBounceSampledDirection, lastBounceHitInfo.normal) / squaredDistance ;
                            }
                            //if(accumulatedBSDFWAConversionFactor == 0.0 || accumulatedGeometryTerms == 0.0 || accumulatedBSDFpdfW == 0.0)
                              //  __debugbreak();

                            Ray nextRay(prevRayHitBundle.hitInfo.intersectionPoint, outgoingDirection);
                            Throughput *= (brdf * glm::dot(outgoingDirection, prevRayHitBundle.hitInfo.normal));
                            accumulatedBSDFpdfW *= pdfBSDF_BSDFSampling;
                            prevRay = nextRay;
                            lastBounceHitInfo = prevRayHitBundle.hitInfo;
                            lastBounceSampledDirection = outgoingDirection;

                        }

                    }
                    else {
                        //Did not hit any emitter so hit env map. Thus get contribution from envmap with losses at material hits
                        //TODO Stop using env map as a special emitter and merge into existing emitter implementation
                        if(glm::any(glm::equal(Throughput, Vector3(0.0f))) || glm::any(glm::isnan(Throughput))) break;
                        L = scene.envMap->Le(prevRay) * Throughput / accumulatedBSDFpdfW;
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