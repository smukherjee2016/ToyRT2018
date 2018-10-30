#pragma once

#include "integrator.hpp"

class PathTracingIntegratorEmitterSampling : public Integrator {
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
                HitInfo prevBounceHitInfo{};
                Vector3 prevBounceSampledDirection(0.0);
                for(int k = 1; k <= numBounces; k++) {
                    std::optional<HitBundle> hitBundle = traceRayReturnClosestHit(prevRay, scene);
                    if (hitBundle) {

                        HitBundle currentHitBundle = hitBundle.value();

                        //If hit the emitter, return its Le multiplied by throughput
                        if(currentHitBundle.closestObject->isEmitter()){
                            if(k == 1) {
                                L += currentHitBundle.closestObject->Le(prevRay); //Direct hit emitter
                            }
                            else {
                                L += Vector3(0.0); //Must not double-count any emitter hit at the end of the path
                            }
                            break;
                        }
                        else {
                            /*
                            * First convert the previous BSDF term into Area domain and accumulate.
                             * Then do emitter sampling at the current point, and finally, BSDF sampling for the next direction.
                            */
                            if(k >= 2) { //Should accumulate previous terms only after the first bounce
                                //Area domain conversion and geometry term from previous bounce
                                Float squaredDistance = glm::length(prevBounceHitInfo.intersectionPoint - currentHitBundle.hitInfo.intersectionPoint) * glm::length(prevBounceHitInfo.intersectionPoint - currentHitBundle.hitInfo.intersectionPoint);
                                Float geometryTerm =  std::max(0.0, glm::dot(prevBounceSampledDirection, prevBounceHitInfo.normal))  //Previous bounce dot product
                                                      * std::max(0.0, glm::dot(currentHitBundle.hitInfo.normal, -prevBounceSampledDirection)) //Current bounce dot product
                                                      / squaredDistance;
                                accumulatedGeometryTerms *= geometryTerm;

                                accumulatedBSDFWAConversionFactor *=  std::max(0.0, glm::dot(currentHitBundle.hitInfo.normal, -prevBounceSampledDirection)) / squaredDistance ;
                            }
                            //if(accumulatedBSDFWAConversionFactor == 0.0 || accumulatedGeometryTerms == 0.0 || accumulatedBSDFpdfW == 0.0)
                            //  __debugbreak();

                            //Emitter Sampling
                            std::optional<std::shared_ptr<Object>> emitterOptionalBundle = scene.selectRandomEmitter();
                            if(emitterOptionalBundle) {
                                std::shared_ptr<Object> emitter = emitterOptionalBundle.value();
                                Point3 pointOnEmitter = emitter->samplePointOnEmitter(-prevRay.d,
                                                                                          currentHitBundle.hitInfo.normal);
                                Vector3 outgoingDirection = glm::normalize(
                                        pointOnEmitter - currentHitBundle.hitInfo.intersectionPoint);
                                Float pdfEmitterA_EmitterSampling = emitter->pdfEmitterA(
                                        currentHitBundle.hitInfo.intersectionPoint);

                                Spectrum brdf = currentHitBundle.closestObject->mat->brdf(outgoingDirection,
                                                                                            -prevRay.d, currentHitBundle.hitInfo.normal);
                                Float pdfBSDF_EmitterSampling = currentHitBundle.closestObject->mat->pdfW(
                                        outgoingDirection, -cameraRay.d,
                                        currentHitBundle.hitInfo.normal);

                                Float tMax = glm::length(pointOnEmitter - currentHitBundle.hitInfo.intersectionPoint) - epsilon;
                                Ray nextRay(currentHitBundle.hitInfo.intersectionPoint, outgoingDirection,Infinity,epsilon,tMax);
                                //Ray nextRay(cameraRayHitBundle.hitInfo.intersectionPoint, outgoingDirection);

                                std::optional<HitBundle> nextRayHitBundle = traceRayReturnClosestHit(nextRay, scene);
                                if (!nextRayHitBundle) {
                                    //Unoccluded so we can reach light source
                                    Vector3 emitterNormal = emitter->getNormalForEmitter(pointOnEmitter);

                                    Float squaredDistance = glm::length(pointOnEmitter - currentHitBundle.hitInfo.intersectionPoint) * glm::length(pointOnEmitter - currentHitBundle.hitInfo.intersectionPoint);
                                    Float geometryTerm =  std::max(0.0, glm::dot(outgoingDirection, currentHitBundle.hitInfo.normal)) * std::max(0.0, glm::dot(emitterNormal, -outgoingDirection))
                                                          / squaredDistance;

                                    Float pdfBSDFA_EmitterSampling = pdfBSDF_EmitterSampling * glm::dot(outgoingDirection, currentHitBundle.hitInfo.normal) / squaredDistance ; //Convert to area domain

                                    Float compositeEmitterPdfA_EmitterSampling = pdfEmitterA_EmitterSampling * scene.pdfSelectEmitter(emitter);

                                    //Float misWeight = PowerHeuristic(1, compositeEmitterPdfA_EmitterSampling, 1, pdfBSDFA_EmitterSampling);

                                    Spectrum accumulatedFactors = Throughput * accumulatedGeometryTerms / (accumulatedBSDFpdfW * accumulatedBSDFWAConversionFactor);
                                    L += emitter->Le(nextRay) * brdf * geometryTerm * accumulatedFactors / compositeEmitterPdfA_EmitterSampling;
                                }


                            }

                            //BSDF Sampling
                            Vector3 outgoingDirection = currentHitBundle.closestObject->mat->sampleDirection(-prevRay.d,
                                                                                                             currentHitBundle.hitInfo.normal);
                            Spectrum brdf = currentHitBundle.closestObject->mat->brdf(outgoingDirection, -prevRay.d,
                                                                                      currentHitBundle.hitInfo.normal);
                            Float pdfBSDF_BSDFSampling = currentHitBundle.closestObject->mat->pdfW(outgoingDirection,
                                                                                                   -prevRay.d,
                                                                                                   currentHitBundle.hitInfo.normal);
                            if(pdfBSDF_BSDFSampling == 0.0)
                                break;


                            Ray nextRay(currentHitBundle.hitInfo.intersectionPoint, outgoingDirection);
                            Throughput *= brdf;
                            accumulatedBSDFpdfW *= pdfBSDF_BSDFSampling;
                            prevRay = nextRay;
                            prevBounceHitInfo = currentHitBundle.hitInfo;
                            prevBounceSampledDirection = outgoingDirection;

                        }

                    }
                    else {
                        //Did not hit any emitter so hit env map. Thus get contribution from envmap with losses at material hits
                        //TODO Stop using env map as a special emitter and merge into existing emitter implementation
                        if(glm::any(glm::equal(Throughput, Vector3(0.0f)))) break;
                        //L = scene.envMap->Le(prevRay) * Throughput * accumulatedGeometryTerms / (accumulatedBSDFpdfW * accumulatedBSDFWAConversionFactor);
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