#pragma once

#include "integrator.hpp"

class PathTracingEmitterv3 : public Integrator {
public:
    void render(const PinholeCamera& pinholeCamera, Film& film, const Scene& scene, const int sampleCount, const int numBounces, const Sampler sampler) const override {
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
                Spectrum L_emitter(0.0);
                Spectrum L_bsdf(0.0);
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

                /*
                 * Design decision: n bounces = n + 1 vertices including the camera vertex.
                 * For emitter sampling, Only connect to emitter till the penultimate vertex for n bounces emitter sampling.
                 * For BSDF sampling, connect the last vertex if and only if it's an emitter.
                */
                //Accumulate path
                for(int k = 1; k <= numBounces; k++) {
                    std::optional<HitBundle> didCurrentRayHitObject = scene.traceRayReturnClosestHit(currentRay, scene);
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
                            Vertex currentVertex;
                            currentVertex.hitPointAndMaterial = currentHitBundle;
                            currentVertex.vertexType = EMITTER;
                            currentVertex.bsdf_xi_xiplus1 = Spectrum(0.0); //Set throughput to 0 for an emitter
                            currentVertex.pdfBSDFW = 1.0;

                            //Keep geometry term to 1 if hit emitter
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

                        if(pdfW == 0.0)
                            break;

                    }
                    else
                        break; //Don't do any (more) bounces if didn't hit anything
                }

                int numVertices = currentSampleBSDFPath.vertices.size();
                //Process the path and fill in geometry term, throughput and area-domain pdf till the penultimate vertex
                for(auto vertexIndex = 0; vertexIndex <= numVertices - 2; vertexIndex++) {
                    if(numVertices > 1) {
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

                        //Store geometry terms and area domain BSDFs *into* the actual vector element, not its copy nextVertex
                        currentSampleBSDFPath.vertices.at(vertexIndex).G_xi_xiplus1 = geometryTerm;
                        currentSampleBSDFPath.vertices.at(vertexIndex).pdfBSDFA = thisVertex.pdfBSDFW * pdfWAConversionFactor_XiPlus1GivenXi;

                    }
                }

                if(numVertices == 2) { //Direct hit emitter
                    L = currentSampleBSDFPath.vertices.at(numVertices - 1).hitPointAndMaterial.closestObject->Le(cameraRay);
                    pixelValue += L;
                    continue;
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
                        auto didShadowRayHitSomething = scene.traceRayReturnClosestHit(shadowRay, scene);
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

                            L_emitter += Le *  attenuationEmitterSampling * bsdfToEmitterPoint * geometryTerm / emitterSamplingPdfA;
                        }
                    }

                    //Update attenuation while going to next vertex due to surface interactions
                    Float geometryTerm = currentVertex.G_xi_xiplus1;
                    Float pdfBSDFA = currentVertex.pdfBSDFA;
                    Spectrum bsdf = currentVertex.bsdf_xi_xiplus1;

                    attenuationEmitterSampling *= (bsdf * geometryTerm / pdfBSDFA);

                }

                //L += L_emitter;

                //If final vertex is on an emitter, add contribution : BSDF sampling
                if(currentSampleBSDFPath.vertices.at(numVertices - 1).vertexType == EMITTER) {
                    Vertex finalVertex = currentSampleBSDFPath.vertices.at(numVertices - 1);
                    Vertex penultimateVertex = currentSampleBSDFPath.vertices.at(numVertices - 2);
                    //Reconstruct final shot ray before hitting the emitter
                    Ray finalBounceRay{};
                    finalBounceRay.o = penultimateVertex.hitPointAndMaterial.hitInfo.intersectionPoint;
                    finalBounceRay.d = glm:: normalize(finalVertex.hitPointAndMaterial.hitInfo.intersectionPoint
                                                       - penultimateVertex.hitPointAndMaterial.hitInfo.intersectionPoint);

                    //Find Le in the given direction of final shot ray
                    L_bsdf = finalVertex.hitPointAndMaterial.closestObject->Le(finalBounceRay);
                    //Calculate light transported along this given path to the camera
                    //Since each vertex contains transport to next vertex, don't need to consider the emitter vertex anymore
                    //This change is also needed to support Emitter Sampling at every point on the path
                    for(int vertexIndex = numVertices - 2; vertexIndex >= 0; vertexIndex--) {
                        Vertex currentVertex = currentSampleBSDFPath.vertices.at(vertexIndex);
                        //Visibility term implicitly 1 along this path
                        Float geometryTerm = currentVertex.G_xi_xiplus1;
                        Float pdfBSDFA = currentVertex.pdfBSDFA;
                        Spectrum bsdf = currentVertex.bsdf_xi_xiplus1;

                        Spectrum attenuation = bsdf * geometryTerm / pdfBSDFA;
                        L_bsdf *= attenuation;

                    }


                }
                L += L_emitter; //Add sample contribution
                pixelValue += L;
            }
            pixelValue /= sampleCount; //Average MC estimation
            film.pixels.at(positionInFilm) = pixelValue; //Write to film
        }

    }

};