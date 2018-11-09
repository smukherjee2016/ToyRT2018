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

                //Add the camera vertex
                HitBundle cameraPointBundle{};
                cameraPointBundle.hitInfo.intersectionPoint = cameraRay.o;
                //cameraPointBundle.closestObject->mat = std::make_shared<TransparentMaterial>();

                //Accumulate path
                for(int k = 1; k <= numBounces; k++) {
                    std::optional<HitBundle> didCurrentRayHitObject = traceRayReturnClosestHit(currentRay, scene);
                    if(didCurrentRayHitObject) {
                        HitBundle currentHitBundle = didCurrentRayHitObject.value();

                        //If hit an emitter, store only the hitBundle object and terminate the path
                        if(currentHitBundle.closestObject->isEmitter()) {
                            Vertex currentVertex;
                            currentVertex.hitPointAndMaterial = currentHitBundle;
                            currentVertex.vertexType = EMITTER;
                            currentVertex.bsdf_xi_xiplus1 = Spectrum(1.0);
                            currentVertex.pdfBSDFW = 1.0;

                            //Keep geometry term and area pdf to 1 if hit emitter
                            currentVertex.G_xi_xiplus1 = 1.0;
                            currentVertex.pdfBSDFA = 1.0;

                            currentSampleBSDFPath.vertices.emplace_back(currentVertex);

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

                        Vertex currentVertex;

                        //Store the hitBundles etc. in the path
                        currentVertex.hitPointAndMaterial = currentHitBundle;
                        currentVertex.bsdf_xi_xiplus1 = bsdf;
                        currentVertex.pdfBSDFW = pdfW;

                        //Keep geometry term, throughput, and area-domain pdf to 1. Will be filled after whole path is constructed
                        currentVertex.pdfBSDFA = 1.0;
                        currentVertex.G_xi_xiplus1 = 1.0;

                        //Push the vertex into the path
                        currentSampleBSDFPath.vertices.emplace_back(currentVertex);

                        //Generate next ray and make it current
                        Ray nextRay(hitPoint, sampledNextBSDFDirection);
                        currentRay = nextRay;

                    }
                    else
                        break; //Don't do any (more) bounces if didn't hit anything
                }

                int pathLength = currentSampleBSDFPath.vertices.size();
                //Process the path and fill in geometry term, throughput and area-domain pdf
                for(auto vertexIndex = 0; vertexIndex < pathLength - 1; vertexIndex++) {
                    if(pathLength > 1) { //Skip the last vertex of the path, TODO it might need special processing for NEE?
                        //Extract current and previous vertices for calculation
                        Vertex thisVertex = currentSampleBSDFPath.vertices.at(vertexIndex);
                        Vertex nextVertex = currentSampleBSDFPath.vertices.at(vertexIndex + 1);

                        Float squaredDistance = glm::length2(nextVertex.hitPointAndMaterial.hitInfo.intersectionPoint - thisVertex.hitPointAndMaterial.hitInfo.intersectionPoint);
                        Vector3 directionThisToNext = glm::normalize(nextVertex.hitPointAndMaterial.hitInfo.intersectionPoint - thisVertex.hitPointAndMaterial.hitInfo.intersectionPoint);
                        Float geometryTerm = std::max(0.0, glm::dot(directionThisToNext, thisVertex.hitPointAndMaterial.hitInfo.normal)) //cos(Theta)_(i-1)
                                * std::max(0.0, glm::dot(-directionThisToNext, nextVertex.hitPointAndMaterial.hitInfo.normal)) //cos(Phi)_i
                                / squaredDistance;

                        Float pdfWAConversionFactor_XiPlus1GivenXi = std::max(0.0, glm::dot(-directionThisToNext, nextVertex.hitPointAndMaterial.hitInfo.normal)) //cos(Phi)_i
                                                               / squaredDistance;

                        //Store geometry terms and area domain BSDFs
                        thisVertex.G_xi_xiplus1 = geometryTerm;
                        thisVertex.pdfBSDFA = thisVertex.pdfBSDFW * pdfWAConversionFactor_XiPlus1GivenXi;

                    }
                }

                //If final vertex is on an emitter, add contribution : BSDF sampling
                if(pathLength >= 1 && currentSampleBSDFPath.vertices.at(pathLength - 1).vertexType == EMITTER) {
                    if(pathLength == 1) { //Direct hit emitter
                        L = currentSampleBSDFPath.vertices.at(pathLength - 1).hitPointAndMaterial.closestObject->Le(cameraRay);
                    }
                    else {
                        Vertex finalVertex = currentSampleBSDFPath.vertices.at(pathLength - 1);
                        Vertex penultimateVertex = currentSampleBSDFPath.vertices.at(pathLength - 2);
                        //Reconstruct final shot ray before hitting the emitter
                        Ray finalBounceRay{};
                        finalBounceRay.o = penultimateVertex.hitPointAndMaterial.hitInfo.intersectionPoint;
                        finalBounceRay.d = glm:: normalize(finalVertex.hitPointAndMaterial.hitInfo.intersectionPoint
                                                           - penultimateVertex.hitPointAndMaterial.hitInfo.intersectionPoint);

                        //Find Le in the given direction of final shot ray
                        L = finalVertex.hitPointAndMaterial.closestObject->Le(finalBounceRay);
                        //Calculate light transported along this given path to the camera
                        for(int vertexIndex = pathLength - 1; vertexIndex >= 0; vertexIndex--) {
                            Vertex currentVertex = currentSampleBSDFPath.vertices.at(vertexIndex);
                            //Visibility term implicitly 1 along this path
                            Float geometryTerm = currentVertex.G_xi_xiplus1;
                            Float pdfBSDFA = currentVertex.pdfBSDFA;
                            Spectrum bsdf = currentVertex.bsdf_xi_xiplus1;

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