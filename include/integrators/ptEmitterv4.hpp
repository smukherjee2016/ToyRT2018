#pragma once

#include "integrator.hpp"

class PathTracingEmitterv4 : public Integrator {
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

                        if(pdfW == 0.0)
                            break;

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

                    }
                    else
                        break; //Don't do any (more) bounces if didn't hit anything
                }

                int numVertices = currentSampleBSDFPath.vertices.size();
                //Process the path and fill in geometry term, throughput and area-domain pdf
                for(auto vertexIndex = 0; vertexIndex < numVertices - 1; vertexIndex++) {
                    if(numVertices > 1) { //Skip the last vertex of the path, TODO it might need special processing for NEE?
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
                }

                //Next Event Estimation aka Emitter Sampling
                //Traverse the path from the first hit (not the sensor vertex) till the penultimate vertex and try to connect to an emitter
                Spectrum attenuationEmitterSampling(1.0);
                for(int vertexIndex = 1; vertexIndex <= (numVertices - 2); vertexIndex++) {
                    //Get the vertex in question. Keep in mind it's only for reading, directly writing to it won't work
                    Vertex currentVertex = currentSampleBSDFPath.vertices.at(vertexIndex);
                    Vertex previousVertex = currentSampleBSDFPath.vertices.at(vertexIndex - 1);

                    Point3 surfacePoint = currentVertex.hitPointAndMaterial.hitInfo.intersectionPoint;
                    Vector3 normalSurfacePoint = currentVertex.hitPointAndMaterial.hitInfo.normal;
                    Vector3 incomingDirectionToSurfacePoint = glm::normalize(currentVertex.hitPointAndMaterial.hitInfo.intersectionPoint
                                                                             - previousVertex.hitPointAndMaterial.hitInfo.intersectionPoint);

                    auto doesSceneHaveEmitters = scene.selectRandomEmitter();
                    if(doesSceneHaveEmitters) {
                        //Select an emitter
                        auto emitter = doesSceneHaveEmitters.value();
                        Float pdfSelectEmitterA = scene.pdfSelectEmitter(emitter);

                        //Sample a point on the emitter
                        Point3 pointOnEmitter = emitter->samplePointOnEmitter();
                        Vector3 normalEmitterPoint = emitter->getNormalForEmitter(pointOnEmitter);
                        Float pdfSelectPointOnEmitterA = emitter->pdfEmitterA(pointOnEmitter);
                        Vector3 directionToEmitter = glm::normalize(pointOnEmitter - surfacePoint);

                        //Construct shadow ray
                        Float distance = glm::length(pointOnEmitter - surfacePoint);
                        Float tMax = distance - epsilon;
                        Ray shadowRay(surfacePoint, directionToEmitter, epsilon, tMax);

                        //Shoot shadow ray to check for visibility. If shadow ray does not hit anything, Visibility Term = 1
                        auto didShadowRayHitSomething = traceRayReturnClosestHit(shadowRay, scene);
                        if(!didShadowRayHitSomething) {
                            //Add contribution from emitter
                            Spectrum bsdfToEmitterPoint = currentVertex.hitPointAndMaterial.closestObject->mat->brdf(directionToEmitter,
                                                                                                                     -incomingDirectionToSurfacePoint, normalSurfacePoint);

                            Float emitterSamplingPdfA = pdfSelectEmitterA * pdfSelectPointOnEmitterA;

                            Float squaredDistance = distance * distance;

                            Float geometryTerm = std::max(0.0, glm::dot(directionToEmitter, normalSurfacePoint)) //cos(Theta), surface point
                                                 * std::max(0.0, glm::dot(-directionToEmitter, normalEmitterPoint)) //cos(Phi), emitter point
                                                 / squaredDistance;

                            Spectrum Le = emitter->Le(shadowRay);

                            L += Le *  attenuationEmitterSampling * bsdfToEmitterPoint * geometryTerm / emitterSamplingPdfA;
                        }
                    }

                    //Update attenuation while going to next vertex due to surface interactions
                    Float geometryTerm = currentVertex.G_xi_xiplus1;
                    Float pdfBSDFA = currentVertex.pdfBSDFA;
                    Spectrum bsdf = currentVertex.bsdf_xi_xiplus1;

                    attenuationEmitterSampling *= (bsdf * geometryTerm / pdfBSDFA);

                }

                pixelValue += L; //Add sample contribution
            }
            pixelValue /= sampleCount; //Average MC estimation
            film.pixels.at(positionInFilm) = pixelValue; //Write to film
        }

    }

};