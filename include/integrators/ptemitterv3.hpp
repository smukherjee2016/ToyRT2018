#pragma once

#include "integrator.hpp"

class PathTracingEmitterv3 : public Integrator {
public:
    void render(const PinholeCamera& pinholeCamera, Film& film, const Scene& scene, const int sampleCount, const int numBounces = 2) const override {
#pragma omp parallel for schedule(dynamic, 1)
        for (int i = 0; i < film.screenHeight * film.screenWidth; i++) {

            int positionInFilm = i;
            int x = positionInFilm % film.screenWidth;
            int y = positionInFilm / film.screenWidth;
            //int positionInFilm = y * film.screenWidth + x;

            Spectrum pixelValue{};
            for (int j = 0; j < sampleCount; j++) {

                Ray cameraRay = pinholeCamera.generateCameraRay(x, y, film);
                Ray currentRay = cameraRay;
                Path currentSampleBSDFPath{};
                Spectrum L(0.0);

                //Accumulate path, assume first vertex on the path is the first hitpoint, not the camera
                for(int k = 1; k <= numBounces; k++) {
                    std::optional<HitBundle> didCurrentRayHitObject = traceRayReturnClosestHit(currentRay, scene);
                    if(didCurrentRayHitObject) {
                        HitBundle currentHitBundle = didCurrentRayHitObject.value();

                        //If hit an emitter, store only the hitBundle object and terminate the path
                        if(currentHitBundle.closestObject->isEmitter()) {
                            currentSampleBSDFPath.isTerminateAtEmitter = true;

                            currentSampleBSDFPath.vertices.emplace_back(currentHitBundle);
                            currentSampleBSDFPath.bsdfs.emplace_back(Vector3(1.0));
                            currentSampleBSDFPath.pdfBSDFWs.emplace_back(1.0);

                            //Keep geometry term, throughput, and area-domain pdf to 1. Will be filled after whole path is constructed
                            currentSampleBSDFPath.pdfBSDFAs.emplace_back(1.0);
                            currentSampleBSDFPath.G_xi_ximinus1s.emplace_back(1.0);
                            break;
                        }

                        //Calculate shading point values
                        Vector3 hitPointNormal = currentHitBundle.hitInfo.normal;
                        Vector3 hitPoint = currentHitBundle.hitInfo.intersectionPoint;

                        //Sample next direction alongwith the pdfs
                        Vector3 sampledNextBSDFDirection = currentHitBundle.closestObject->mat->sampleDirection(-currentRay.d, hitPointNormal);
                        Spectrum bsdf = currentHitBundle.closestObject->mat->brdf(sampledNextBSDFDirection, -currentRay.d, hitPointNormal);
                        Float pdfW = currentHitBundle.closestObject->mat->pdfW(sampledNextBSDFDirection, -currentRay.d, hitPointNormal);

                        if(pdfW == 0.0)
                            break;

                        //Store the hitBundles etc. in the path
                        currentSampleBSDFPath.vertices.emplace_back(currentHitBundle);
                        currentSampleBSDFPath.bsdfs.emplace_back(bsdf);
                        currentSampleBSDFPath.pdfBSDFWs.emplace_back(pdfW);

                        //Keep geometry term, throughput, and area-domain pdf to 1. Will be filled after whole path is constructed
                        currentSampleBSDFPath.pdfBSDFAs.emplace_back(1.0);
                        currentSampleBSDFPath.G_xi_ximinus1s.emplace_back(1.0);

                        //Generate next ray and make it current
                        Ray nextRay(hitPoint, sampledNextBSDFDirection);
                        currentRay = nextRay;

                    }
                    else
                        break; //Don't do any (more) bounces if didn't hit anything
                }

                auto pathLength = currentSampleBSDFPath.vertices.size();
                //Process the path and fill in geometry term, throughput and area-domain pdf
                for(auto vertexIndex = 0; vertexIndex < pathLength; vertexIndex++) {
                    if(pathLength >= 1 && vertexIndex >= 1) { //Only start processing from the 2nd vertex, should cancel out for 1st vertex and camera
                        //Extract current and previous vertices for calculation
                        HitBundle thisVertex = currentSampleBSDFPath.vertices.at(vertexIndex);
                        HitBundle prevVertex = currentSampleBSDFPath.vertices.at(vertexIndex - 1);

                        Float squaredDistance = glm::length2(thisVertex.hitInfo.intersectionPoint - prevVertex.hitInfo.intersectionPoint);
                        Vector3 directionPrevToThis = glm::normalize(thisVertex.hitInfo.intersectionPoint - prevVertex.hitInfo.intersectionPoint);
                        Float geometryTerm = std::max(0.0, glm::dot(directionPrevToThis, prevVertex.hitInfo.normal)) //cos(Theta)_(i-1)
                                * std::max(0.0, glm::dot(-directionPrevToThis, thisVertex.hitInfo.normal)) //cos(Phi)_i
                                / squaredDistance;

                        Float pdfWAConversionFactor_XiPlus1GivenXi = std::max(0.0, glm::dot(-directionPrevToThis, thisVertex.hitInfo.normal)) //cos(Phi)_i
                                                               / squaredDistance;

                        Float pdfA_XiPlus1GivenXi = currentSampleBSDFPath.pdfBSDFWs.at(vertexIndex) * pdfWAConversionFactor_XiPlus1GivenXi;

                        //Store geometry terms and area domain BSDFs
                        currentSampleBSDFPath.G_xi_ximinus1s.at(vertexIndex) = geometryTerm;
                        currentSampleBSDFPath.pdfBSDFAs.at(vertexIndex) = pdfA_XiPlus1GivenXi;

                    }
                }

                //If final vertex is on an emitter, add contribution : BSDF sampling
                if(pathLength >= 1 && currentSampleBSDFPath.isTerminateAtEmitter) {
                    if(pathLength == 1) { //Direct hit emitter
                        L = currentSampleBSDFPath.vertices.at(pathLength - 1).closestObject->Le(cameraRay);
                    }
                    else {
                        //Reconstruct final shot ray before hitting the emitter
                        Ray finalBounceRay{};
                        finalBounceRay.o = currentSampleBSDFPath.vertices.at(pathLength - 2).hitInfo.intersectionPoint;
                        finalBounceRay.d = glm:: normalize(currentSampleBSDFPath.vertices.at(pathLength - 1).hitInfo.intersectionPoint
                                                           - currentSampleBSDFPath.vertices.at(pathLength - 2).hitInfo.intersectionPoint);

                        //Find Le in the given direction of final shot ray
                        L = currentSampleBSDFPath.vertices.at(pathLength - 1).closestObject->Le(finalBounceRay);
                        //Calculate light transported along this given path to the camera
                        for(int vertexIndex = pathLength - 1; vertexIndex >= 0; vertexIndex--) {
                            //Visibility term implicitly 1 along this path
                            Float geometryTerm = currentSampleBSDFPath.G_xi_ximinus1s.at(vertexIndex);
                            Float pdfBSDFA = currentSampleBSDFPath.pdfBSDFAs.at(vertexIndex);
                            Spectrum bsdf = currentSampleBSDFPath.bsdfs.at(vertexIndex);

                            Spectrum attenuation = bsdf * geometryTerm / pdfBSDFA;
                            L *= attenuation;

                        }
                    }

                }
                pixelValue += L; //Add sample contribution
            }
            pixelValue /= sampleCount; //Average MC estimation
            film.pixels.at(positionInFilm) = pixelValue; //Write to film
        }

    }

};