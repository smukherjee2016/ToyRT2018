#pragma once

#include <thread>
#include "integrator.hpp"

class PathTracingEmitterv4 : public Integrator {
public:
    void render(std::shared_ptr<Camera> camera, std::shared_ptr<Film> film, Scene &scene, const int sampleCount,
                const int numBounces = 2) const override {
        tbb::task_scheduler_init init(tbb::task_scheduler_init::default_num_threads() - 1);  // Explicit number of threads
        //Bottom-up scanlining
        tbb::parallel_for(tbb::blocked_range<size_t>(0, film->screenHeight * film->screenWidth),  [=, &scene](const tbb::blocked_range<size_t>& r) {

            for (size_t i = r.begin(); i != r.end(); ++i) {
                int positionInFilm = i;
                int x = positionInFilm % film->screenWidth;
                int y = positionInFilm / film->screenWidth;
                //int positionInFilm = y * film->screenWidth + x;

                Spectrum pixelValue{};
                for (int j = 0; j < sampleCount; j++) {

                    Ray cameraRay = camera->generateCameraRay(x, y, film);
                    Ray currentRay = cameraRay;
                    Path currentSampleBSDFPath{};
                    Spectrum L(0.0);

                    //Add the camera vertex, a tiny tiny transparent sphere
                    HitBundle cameraPointBundle{};
                    cameraPointBundle.hitInfo.intersectionPoint = cameraRay.o;
                    cameraPointBundle.closestObject = std::make_unique<Sphere>(cameraRay.o, epsilon,
                                                                               std::make_shared<TransparentMaterial>());
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
                    for (int k = 1; k <= numBounces; k++) {
                        std::optional<HitBundle> didCurrentRayHitObject = scene.traceRayReturnClosestHit(currentRay);
                        if (didCurrentRayHitObject) {
                            HitBundle currentHitBundle = didCurrentRayHitObject.value();

                            //Special processing for the sensor vertex: Set normal to be the same direction as towards
                            //the first hitPoint so that cos(theta) = 1 and cos(phi)/d^2 and the geoTerm cancel out
                            if (k == 1 && currentSampleBSDFPath.vertices.at(0).vertexType == SENSOR) {
                                Point3 cameraPosition = currentSampleBSDFPath.vertices.at(
                                        0).hitPointAndMaterial.hitInfo.intersectionPoint;
                                Point3 firstHitPointPosition = currentHitBundle.hitInfo.intersectionPoint;

                                currentSampleBSDFPath.vertices.at(0).hitPointAndMaterial.hitInfo.normal
                                        = glm::normalize(firstHitPointPosition - cameraPosition);


                            }

                            //If hit an emitter, store only the hitBundle object and terminate the path
                            if (currentHitBundle.closestObject->isEmitter()) {
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
                            Vector3 sampledNextBSDFDirection = currentHitBundle.closestObject->mat->sampleDirection(
                                    -currentRay.d, hitPointNormal);
                            Spectrum bsdf = currentHitBundle.closestObject->mat->brdf(sampledNextBSDFDirection,
                                                                                      -currentRay.d, hitPointNormal);
                            Float pdfW = currentHitBundle.closestObject->mat->pdfW(sampledNextBSDFDirection,
                                                                                   -currentRay.d, hitPointNormal);

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

                            if (pdfW == 0.0)
                                break;

                        } else
                            break; //Don't do any (more) bounces if didn't hit anything
                    }

                    int numVertices = currentSampleBSDFPath.vertices.size();
                    //Process the path and fill in geometry term, throughput and area-domain pdf
                    for (auto vertexIndex = 0; vertexIndex < numVertices - 1; vertexIndex++) {
                        //Extract current and previous vertices for calculation
                        HitBundle thisVertex = currentSampleBSDFPath.vertices.at(vertexIndex).hitPointAndMaterial;
                        HitBundle nextVertex = currentSampleBSDFPath.vertices.at(vertexIndex + 1).hitPointAndMaterial;

                        Float squaredDistance = glm::length2(
                                nextVertex.hitInfo.intersectionPoint - thisVertex.hitInfo.intersectionPoint);
                        Vector3 directionThisToNext = glm::normalize(
                                nextVertex.hitInfo.intersectionPoint - thisVertex.hitInfo.intersectionPoint);
                        Float geometryTerm = std::max(0.0, glm::dot(directionThisToNext,
                                                                    thisVertex.hitInfo.normal)) //cos(Theta)_(i-1)
                                             * std::max(0.0, glm::dot(-directionThisToNext,
                                                                      nextVertex.hitInfo.normal)) //cos(Phi)_i
                                             / squaredDistance;

                        Float pdfWAConversionFactor_XiPlus1GivenXi =
                                std::max(0.0, glm::dot(-directionThisToNext, nextVertex.hitInfo.normal)) //cos(Phi)_i
                                / squaredDistance;

                        Float pdfA_XiPlus1GivenXi = currentSampleBSDFPath.vertices.at(vertexIndex).pdfBSDFW *
                                                    pdfWAConversionFactor_XiPlus1GivenXi;

                        //Store geometry terms and area domain BSDFs
                        currentSampleBSDFPath.vertices.at(vertexIndex).G_xi_xiplus1 = geometryTerm;
                        currentSampleBSDFPath.vertices.at(vertexIndex).pdfBSDFA = pdfA_XiPlus1GivenXi;

                    }

                    if (numVertices ==
                        2) { //Direct hit emitter Vertex lastVertex = currentSampleBSDFPath.vertices.at(numVertices - 1);
                        Vertex lastVertex = currentSampleBSDFPath.vertices.at(numVertices - 1);
                        if (lastVertex.vertexType == EMITTER)
                            L = lastVertex.hitPointAndMaterial.closestObject->emitter->Le(cameraRay);
                        pixelValue += L;
                        continue;
                    }

                    //Emitter sampling
                    Spectrum attenuation(1.0);
                    for (int vertexIndex = 1; vertexIndex <= (numVertices - 2); vertexIndex++) {
                        auto doesSceneHaveEmitters = scene.selectRandomEmitter();
                        if (doesSceneHaveEmitters) {
                            auto emitter = doesSceneHaveEmitters.value();
                            Float pdfSelectEmitter = scene.pdfSelectEmitter(emitter);
                            Vector3 incomingDirectionToVertex = glm::normalize(currentSampleBSDFPath.vertices.at(
                                    vertexIndex).hitPointAndMaterial.hitInfo.intersectionPoint
                                                                               - currentSampleBSDFPath.vertices.at(
                                    vertexIndex - 1).hitPointAndMaterial.hitInfo.intersectionPoint);

                            //Sample point on emitter
                            Point3 pointOnEmitter = emitter->samplePointOnEmitter(Sampler());
                            Vector3 emitterPointNormal = emitter->getNormalForEmitter(pointOnEmitter);
                            Point3 currentVertexPos = currentSampleBSDFPath.vertices.at(
                                    vertexIndex).hitPointAndMaterial.hitInfo.intersectionPoint;
                            Vector3 directionToEmitter = glm::normalize(pointOnEmitter - currentVertexPos);
                            Float distanceToEmitter = glm::distance(pointOnEmitter, currentVertexPos);
                            Float tMax = distanceToEmitter - epsilon;

                            Ray shadowRay(currentVertexPos, directionToEmitter, epsilon, tMax);
                            auto didShadowRayHitSomething = scene.traceRayReturnClosestHit(shadowRay);
                            if (!didShadowRayHitSomething) {
                                Vector3 currentVertexNormal = currentSampleBSDFPath.vertices.at(
                                        vertexIndex).hitPointAndMaterial.hitInfo.normal;
                                Float squaredDistance = glm::distance2(pointOnEmitter, currentVertexPos);
                                Float geometryTerm =
                                        std::max(0.0, glm::dot(currentVertexNormal, directionToEmitter)) //cos(Theta)
                                        * std::max(0.0, glm::dot(emitterPointNormal, -directionToEmitter)) //cos(Phi)
                                        / squaredDistance;

                                Spectrum bsdfEmitter = currentSampleBSDFPath.vertices.at(
                                        vertexIndex).hitPointAndMaterial.closestObject->mat->brdf(
                                        -incomingDirectionToVertex, directionToEmitter, currentVertexNormal);
                                Float pdfEmitterSampling =
                                        pdfSelectEmitter * emitter->pdfSelectPointOnEmitterA(pointOnEmitter);
                                Spectrum Le = emitter->Le(shadowRay);

                                L += attenuation * Le * bsdfEmitter * geometryTerm / pdfEmitterSampling;

                            }

                            Spectrum bsdfCurrentVertex = currentSampleBSDFPath.vertices.at(vertexIndex).bsdf_xi_xiplus1;
                            Float geometryTerm = currentSampleBSDFPath.vertices.at(vertexIndex).G_xi_xiplus1;
                            Float pdfA = currentSampleBSDFPath.vertices.at(vertexIndex).pdfBSDFA;

                            attenuation *= ((bsdfCurrentVertex * geometryTerm) / pdfA);

                        }
                    }
                    if (L.x < 0.0 || L.y < 0.0 || L.z < 0.0)
                        throw std::runtime_error("Encountered negative value!");

                    pixelValue += L; //Add sample contribution
                }
                pixelValue /= sampleCount; //Average MC estimation
                film->pixels.at(positionInFilm) = pixelValue; //Write to film
            }
        });

    }

};