#pragma once

#include <thread>
#include <omp.h>
#include "integrator.hpp"
#include "path/pathsampler.hpp"

class PathTracingIntegratorv4 : public Integrator {
public:
    void render(const PinholeCamera& pinholeCamera, Film& film, const Scene& scene, const int sampleCount, const int numBounces = 2) const override {
        unsigned int numThreads = std::thread::hardware_concurrency() - 1;
#pragma omp parallel for schedule(dynamic, 1) num_threads(numThreads)
//#pragma omp parallel for schedule(dynamic, 1)

        for (int i = 0; i < film.screenHeight * film.screenWidth; i++) {

            int positionInFilm = i;
            int x = positionInFilm % film.screenWidth;
            int y = positionInFilm / film.screenWidth;
            //int positionInFilm = y * film.screenWidth + x;

            Spectrum pixelValue{};
            for (int j = 0; j < sampleCount; j++) {

                Ray cameraRay = pinholeCamera.generateCameraRay(x, y, film);
                Spectrum L(0.0);
                Spectrum L_emitter(0.0);
                Spectrum L_BSDF(0.0);

                PathSampler pathSampler;
                Path currentSampleBSDFPath = pathSampler.generatePath(scene, cameraRay, numBounces);

                int numVertices = currentSampleBSDFPath.vertices.size();

                if(numVertices == 2) { //Direct hit emitter
                    L = currentSampleBSDFPath.vertices.at(numVertices - 1).hitPointAndMaterial.closestObject->Le(cameraRay);
                    pixelValue += L;
                    continue;
                }

                //Emitter sampling
                Spectrum attenuation(1.0);
                for(int vertexIndex = 1; vertexIndex <= (numVertices - 2); vertexIndex++) {
                    auto doesSceneHaveEmitters = scene.selectRandomEmitter();
                    if(doesSceneHaveEmitters) {
                        auto emitter = doesSceneHaveEmitters.value();
                        Float pdfSelectEmitter = scene.pdfSelectEmitter(emitter);
                        auto currentHitPoint = currentSampleBSDFPath.vertices.at(vertexIndex).hitPointAndMaterial;
                        auto previousHitPoint = currentSampleBSDFPath.vertices.at(vertexIndex - 1).hitPointAndMaterial;

                        Vector3 incomingDirectionToVertex = glm::normalize(currentHitPoint.hitInfo.intersectionPoint
                                                                           - previousHitPoint.hitInfo.intersectionPoint);

                        //Sample point on emitter
                        Point3 pointOnEmitter = emitter->samplePointOnEmitter();
                        Vector3 emitterPointNormal = emitter->getNormalForEmitter(pointOnEmitter);
                        Point3 currentVertexPos = currentHitPoint.hitInfo.intersectionPoint;
                        Vector3 directionToEmitter = glm::normalize(pointOnEmitter - currentVertexPos);
                        Float distanceToEmitter = glm::distance(pointOnEmitter, currentVertexPos);
                        Float tMax = distanceToEmitter - epsilon;

                        Ray shadowRay(currentVertexPos, directionToEmitter, epsilon, tMax);
                        auto didShadowRayHitSomething = traceRayReturnClosestHit(shadowRay, scene);
                        if(!didShadowRayHitSomething) {
                            Vector3 currentVertexNormal = glm::normalize(currentHitPoint.hitInfo.normal);
                            Float squaredDistance = glm::distance2(pointOnEmitter, currentVertexPos);
                            Float geometryTerm = std::max(0.0, glm::dot(currentVertexNormal, directionToEmitter)) //cos(Theta)
                                                 * std::max(0.0, glm::dot(emitterPointNormal, -directionToEmitter)) //cos(Phi)
                                                 / squaredDistance;

                            Spectrum bsdfEmitter = currentHitPoint.closestObject->mat->brdf(
                                    directionToEmitter, -incomingDirectionToVertex, currentVertexNormal);
                            Float pdfEmitterA_EmitterSampling = pdfSelectEmitter * emitter->pdfEmitterA(pointOnEmitter);
                            Spectrum Le = emitter->Le(shadowRay);

                            //MIS Calculations
                            Float pdfBSDFW_EmitterSampling = currentHitPoint.closestObject->mat->pdfW(
                                    directionToEmitter, -incomingDirectionToVertex, currentVertexNormal);
                            Float pdfBSDFA_EmitterSampling = pdfBSDFW_EmitterSampling * std::max(0.0, glm::dot(emitterPointNormal, -directionToEmitter)) / squaredDistance;
                            Float misWeight = PowerHeuristic(pdfEmitterA_EmitterSampling, pdfBSDFA_EmitterSampling);

                            Spectrum bob = attenuation * Le * bsdfEmitter * geometryTerm * misWeight / pdfEmitterA_EmitterSampling;
                            if(glm::any(glm::lessThan(bob, Vector3(0.0))))
                                std::cout << "lel";
                            L_emitter += bob;

                        }

                        Spectrum bsdfCurrentVertex = currentSampleBSDFPath.vertices.at(vertexIndex).bsdf_xi_xiplus1;
                        Float geometryTerm = currentSampleBSDFPath.vertices.at(vertexIndex).G_xi_xiplus1;
                        Float pdfA = currentSampleBSDFPath.vertices.at(vertexIndex).pdfBSDFA;

                        attenuation *= ((bsdfCurrentVertex * geometryTerm) / pdfA);

                    }
                }

                //If final vertex is on an emitter, add contribution : BSDF sampling
                if(currentSampleBSDFPath.vertices.at(numVertices - 1).vertexType == EMITTER) {
                    //Reconstruct final shot ray before hitting the emitter
                    Vertex finalVertexOnEmitter = currentSampleBSDFPath.vertices.at(numVertices - 1);
                    Vertex penultimateVertex = currentSampleBSDFPath.vertices.at(numVertices - 2);

                    Ray finalBounceRay{};
                    finalBounceRay.o = penultimateVertex.hitPointAndMaterial.hitInfo.intersectionPoint;
                    finalBounceRay.d = glm::normalize(finalVertexOnEmitter.hitPointAndMaterial.hitInfo.intersectionPoint
                                                      - penultimateVertex.hitPointAndMaterial.hitInfo.intersectionPoint);

                    //Find Le in the given direction of final shot ray
                    L_BSDF = finalVertexOnEmitter.hitPointAndMaterial.closestObject->Le(
                            finalBounceRay);

                    //MIS Calculation for BSDF sampling. Weight only between the final and penultimate vertex, where final vertex is an emitter
                    //For all other bounces, no emitter to sample from, so BSDF sampling will implicitly have a weight of 1.0
                    Point3 pointOnEmitter = finalVertexOnEmitter.hitPointAndMaterial.hitInfo.intersectionPoint;
                    auto emitter = finalVertexOnEmitter.hitPointAndMaterial.closestObject;
                    Float pdfSelectEmitterA = scene.pdfSelectEmitter(emitter);
                    Float pdfSelectFinalVertexOnEmitterA = emitter->pdfEmitterA(pointOnEmitter);
                    Float pdfEmitterA_BSDFSampling = pdfSelectEmitterA * pdfSelectFinalVertexOnEmitterA;
                    Float pdfBSDFA_BSDFSampling = penultimateVertex.pdfBSDFA; //Since the pdf of hitting the next vertex is stored in the previous vertex now
                    Float misWeight = PowerHeuristic(pdfBSDFA_BSDFSampling, pdfEmitterA_BSDFSampling);

                    //Attenuate L by the weight
                    L_BSDF *= misWeight;

                    //Calculate light transported along this given path to the camera
                    for (int vertexIndex = numVertices - 2; vertexIndex >= 0; vertexIndex--) {
                        //Visibility term implicitly 1 along this path
                        Float geometryTerm = currentSampleBSDFPath.vertices.at(vertexIndex).G_xi_xiplus1;
                        Float pdfBSDFA = currentSampleBSDFPath.vertices.at(vertexIndex).pdfBSDFA;
                        Spectrum bsdf = currentSampleBSDFPath.vertices.at(vertexIndex).bsdf_xi_xiplus1;

                        Spectrum attenuation = bsdf * geometryTerm / pdfBSDFA;
                        L_BSDF *= attenuation;

                    }
                }

                //L = L_BSDF;
                L += L_emitter + L_BSDF;

                pixelValue += L; //Add sample contribution
            }
            pixelValue /= sampleCount; //Average MC estimation
            film.pixels.at(positionInFilm) = pixelValue; //Write to film

        auto tid = omp_get_thread_num();
        if(tid == 0 && x % film.screenWidth == 0)
            std::cout << "Completed " << (static_cast<Float>(positionInFilm) / (film.screenWidth * film.screenHeight)) * 100 << " percent.\n";
        }

    }

};