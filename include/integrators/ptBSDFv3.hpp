#pragma once

#include <thread>
#include "integrator.hpp"

class PathTracingBSDFv3 : public Integrator {
public:
    void render(const PinholeCamera &pinholeCamera, Film &film, Scene &scene, const int sampleCount,
                const int numBounces = 2) const override {
        unsigned int numThreads = std::thread::hardware_concurrency() - 1;
#pragma omp parallel for schedule(dynamic, 1) num_threads(numThreads)
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

                //Add the camera vertex, a tiny tiny transparent sphere
                HitBundle cameraPointBundle{};
                cameraPointBundle.hitInfo.intersectionPoint = cameraRay.o;
                cameraPointBundle.closestObject = std::make_unique<Sphere>(cameraRay.o, epsilon , std::make_shared<TransparentMaterial>());
                //Fill in normal, geoTerm and pdfA later so they cancel out

                Vertex cameraVertex{};
                cameraVertex.hitPointAndMaterial = cameraPointBundle;
                cameraVertex.pdfBSDFW = 1.0; //Probability of sampling this vertex in SA domain is always 1
                cameraVertex.bsdf_xi_xiplus1 = Spectrum(1.0); //Pass all the light through
                //Set pdfA and Geoterm later
                cameraVertex.pdfBSDFA = 1.0;
                cameraVertex.G_xi_xiplus1 = 1.0;

                //Set camera  vertex to have SENSOR type
                cameraVertex.vertexType = SENSOR;


                currentSampleBSDFPath.vertices.emplace_back(cameraVertex);

                //Accumulate path, assume first vertex on the path is the first hitpoint, not the camera
                for(int k = 1; k <= numBounces; k++) {
                    std::optional<HitBundle> didCurrentRayHitObject = traceRayReturnClosestHit(currentRay);
                    if(didCurrentRayHitObject) {
                        HitBundle currentHitBundle = didCurrentRayHitObject.value();

                        //Special processing for the sensor vertex: Set normal to be the same direction as towards
                        //the first hitPoint so that cos(theta) = 1 and cos(phi)/d^2 and the geoTerm cancel out
                        if(k == 1 && currentSampleBSDFPath.vertices.at(0).vertexType == SENSOR) {
                            Point3 cameraPosition = currentSampleBSDFPath.vertices.at(0).hitPointAndMaterial.hitInfo.intersectionPoint;
                            Point3 firstHitPointPosition = currentHitBundle.hitInfo.intersectionPoint;

                            currentSampleBSDFPath.vertices.at(0).hitPointAndMaterial.hitInfo.normal
                                    = glm::normalize(firstHitPointPosition - cameraPosition);


                        }

                        //If hit an emitter, store only the hitBundle object and terminate the path
                        if(currentHitBundle.closestObject->isEmitter()) {
                            Vertex emitterVertex{};
                            emitterVertex.vertexType = EMITTER;

                            emitterVertex.hitPointAndMaterial = currentHitBundle;
                            emitterVertex.bsdf_xi_xiplus1 = Vector3(1.0);
                            emitterVertex.pdfBSDFW = 1.0;

                            //Keep geometry term and area pdf to 1 if hit emitter
                            emitterVertex.pdfBSDFA = 1.0;
                            emitterVertex.G_xi_xiplus1 = 1.0;

                            currentSampleBSDFPath.vertices.emplace_back(emitterVertex);
                            break;
                        }

                        //Calculate shading point values
                        Vector3 hitPointNormal = currentHitBundle.hitInfo.normal;
                        Vector3 hitPoint = currentHitBundle.hitInfo.intersectionPoint;

                        //Sample next direction alongwith the pdfs
                        Vector3 sampledNextBSDFDirection = currentHitBundle.closestObject->mat->sampleDirection(-currentRay.d, hitPointNormal);
                        Spectrum bsdf = currentHitBundle.closestObject->mat->brdf(sampledNextBSDFDirection, -currentRay.d, hitPointNormal);
                        Float pdfW = currentHitBundle.closestObject->mat->pdfW(sampledNextBSDFDirection, -currentRay.d, hitPointNormal);

                        Vertex currentVertex{};
                        //Store the hitBundles etc. in the path
                        currentVertex.hitPointAndMaterial = currentHitBundle;
                        currentVertex.bsdf_xi_xiplus1 = bsdf;
                        currentVertex.pdfBSDFW = pdfW;

                        //Keep geometry term, throughput, and area-domain pdf to 1. Will be filled after whole path is constructed
                        currentVertex.pdfBSDFA = 1.0;
                        currentVertex.G_xi_xiplus1 = 1.0;

                        currentSampleBSDFPath.vertices.emplace_back(currentVertex);
                        //Generate next ray and make it current
                        Ray nextRay(hitPoint, sampledNextBSDFDirection);
                        currentRay = nextRay;

                        if(pdfW == 0.0)
                            break;

                    }
                    else
                        break; //Don't do any (more) bounces if didn't hit anything
                }

                int numVertices = currentSampleBSDFPath.vertices.size();
                //Process the path and fill in geometry term, throughput and area-domain pdf
                for(auto vertexIndex = 0; vertexIndex < numVertices - 1; vertexIndex++) {
                    //Extract current and previous vertices for calculation
                    HitBundle thisVertex = currentSampleBSDFPath.vertices.at(vertexIndex).hitPointAndMaterial;
                    HitBundle nextVertex = currentSampleBSDFPath.vertices.at(vertexIndex + 1).hitPointAndMaterial;

                    Float squaredDistance = glm::length2(nextVertex.hitInfo.intersectionPoint - thisVertex.hitInfo.intersectionPoint);
                    Vector3 directionThisToNext = glm::normalize(nextVertex.hitInfo.intersectionPoint - thisVertex.hitInfo.intersectionPoint);
                    Float geometryTerm = std::max(0.0, glm::dot(directionThisToNext, thisVertex.hitInfo.normal)) //cos(Theta)_(i-1)
                                         * std::max(0.0, glm::dot(-directionThisToNext, nextVertex.hitInfo.normal)) //cos(Phi)_i
                                         / squaredDistance;

                    Float pdfWAConversionFactor_XiPlus1GivenXi = std::max(0.0, glm::dot(-directionThisToNext, nextVertex.hitInfo.normal)) //cos(Phi)_i
                                                                 / squaredDistance;

                    Float pdfA_XiPlus1GivenXi = currentSampleBSDFPath.vertices.at(vertexIndex).pdfBSDFW * pdfWAConversionFactor_XiPlus1GivenXi;

                    //Store geometry terms and area domain BSDFs
                    currentSampleBSDFPath.vertices.at(vertexIndex).G_xi_xiplus1 = geometryTerm;
                    currentSampleBSDFPath.vertices.at(vertexIndex).pdfBSDFA = pdfA_XiPlus1GivenXi;

                }

                //If final vertex is on an emitter, add contribution : BSDF sampling
                if(numVertices >= 1 && currentSampleBSDFPath.vertices.at(numVertices - 1).vertexType == EMITTER) {
                    if (numVertices == 1) { //Direct hit emitter
                        L = currentSampleBSDFPath.vertices.at(numVertices - 1).hitPointAndMaterial.closestObject->emitter->Le(
                                cameraRay);
                    } else {
                        //Reconstruct final shot ray before hitting the emitter
                        Ray finalBounceRay{};
                        finalBounceRay.o = currentSampleBSDFPath.vertices.at(
                                numVertices - 2).hitPointAndMaterial.hitInfo.intersectionPoint;
                        finalBounceRay.d = glm::normalize(currentSampleBSDFPath.vertices.at(
                                numVertices - 1).hitPointAndMaterial.hitInfo.intersectionPoint
                                                          - currentSampleBSDFPath.vertices.at(
                                numVertices - 2).hitPointAndMaterial.hitInfo.intersectionPoint);

                        //Find Le in the given direction of final shot ray
                        L = currentSampleBSDFPath.vertices.at(numVertices - 1).hitPointAndMaterial.closestObject->emitter->Le(
                                finalBounceRay);
                        //Calculate light transported along this given path to the camera
                        for (int vertexIndex = numVertices - 1; vertexIndex >= 1; vertexIndex--) {
                            //Visibility term implicitly 1 along this path
                            Float geometryTerm = currentSampleBSDFPath.vertices.at(vertexIndex).G_xi_xiplus1;
                            Float pdfBSDFA = currentSampleBSDFPath.vertices.at(vertexIndex).pdfBSDFA;
                            Spectrum bsdf = currentSampleBSDFPath.vertices.at(vertexIndex).bsdf_xi_xiplus1;

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