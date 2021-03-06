#pragma once

#include "integrators/integrator.hpp"

typedef std::optional<HitBundle> HitBundleOptional;

class PathTracingEmitterv2 : public Integrator {

public:
    void render(const PinholeCamera &pinholeCamera, Film &film, Scene &scene, const int sampleCount,
                const int numBounces) const {
#pragma omp parallel for schedule(dynamic, 1)
        for (int i = 0; i < film.screenHeight * film.screenWidth; i++) {

            int positionInFilm = i;
            int x = positionInFilm % film.screenWidth;
            int y = positionInFilm / film.screenWidth;
            //int positionInFilm = y * film.screenWidth + x;

            Spectrum pixelValue(0.0);
            for (int j = 0; j < sampleCount; j++) {
                Spectrum L(0.0);
                Spectrum throughput(1.0);

                Ray thisRay = pinholeCamera.generateCameraRay(x, y, film);
                HitBundleOptional hitInformationBundleOptional = scene.traceRayReturnClosestHit(thisRay);

                if(hitInformationBundleOptional) {
                    HitBundle hitInfoBundle_Bounce1 = hitInformationBundleOptional.value();
                    if(hitInfoBundle_Bounce1.closestObject->isEmitter())
                    {
                        //L += hitInfoBundle_Bounce1.closestObject->Le(thisRay);
                        L += Vector3(0.0);
                        pixelValue += L;
                        continue;
                    }

                    Vector3 sampledNextDirection = hitInfoBundle_Bounce1.closestObject->mat->sampleDirection(-thisRay.d, hitInfoBundle_Bounce1.hitInfo.normal);
                    Spectrum bsdf = hitInfoBundle_Bounce1.closestObject->mat->brdf(sampledNextDirection, -thisRay.d, hitInfoBundle_Bounce1.hitInfo.normal);
                    Float pdfBSDFW_1 = hitInfoBundle_Bounce1.closestObject->mat->pdfW(sampledNextDirection, -thisRay.d, hitInfoBundle_Bounce1.hitInfo.normal);

                    if(pdfBSDFW_1 == 0.0)
                        continue;

                    throughput *= (bsdf / pdfBSDFW_1);

                    Ray ray_2(hitInfoBundle_Bounce1.hitInfo.intersectionPoint, sampledNextDirection);
                    HitBundleOptional hitInformationBundleOptional2 = scene.traceRayReturnClosestHit(ray_2);

                    //Bounce 2
                    if(hitInformationBundleOptional2) {
                        HitBundle hitInfoBundle_Bounce2 = hitInformationBundleOptional2.value();
                        if(hitInfoBundle_Bounce2.closestObject->isEmitter())
                        {
                            L += Vector3(0.0);
                            pixelValue += L;
                            continue;
                        }

                        //Emitter sampling
                        std::optional<std::shared_ptr<Emitter>> emitterOptional = scene.selectRandomEmitter();
                        if(emitterOptional) { //Found an emitter
                            std::shared_ptr<Emitter> emitter = emitterOptional.value();

                            Point3 sampledPointOnEmitter = emitter->samplePointOnEmitter(Sampler());
                            Vector3 outgoingDirection_Emitter = glm::normalize(sampledPointOnEmitter - hitInfoBundle_Bounce2.hitInfo.intersectionPoint);
                            Spectrum bsdf_2 = hitInfoBundle_Bounce2.closestObject->mat->brdf(outgoingDirection_Emitter, -ray_2.d, hitInfoBundle_Bounce2.hitInfo.normal);

                            Float tMax = glm::length(sampledPointOnEmitter - hitInfoBundle_Bounce2.hitInfo.intersectionPoint) - epsilon;
                            Ray shadowRay(hitInfoBundle_Bounce2.hitInfo.intersectionPoint, outgoingDirection_Emitter, epsilon, tMax);
                            HitBundleOptional shadowRayHitSomething = scene.traceRayReturnClosestHit(shadowRay);
                            if(!shadowRayHitSomething) {
                                Vector3 emitterNormal = emitter->getNormalForEmitter(sampledPointOnEmitter);
                                Float squaredDistance = glm::length2(sampledPointOnEmitter - hitInfoBundle_Bounce2.hitInfo.intersectionPoint);
                                Float geometryTerm = std::max(0.0, glm::dot(-outgoingDirection_Emitter, emitterNormal)) //cos(phi)
                                        * std::max(0.0, glm::dot(outgoingDirection_Emitter, hitInfoBundle_Bounce2.hitInfo.normal)) //cos(theta)
                                        / squaredDistance;

                                Float pdfEmitterA_2 = emitter->pdfSelectPointOnEmitterA(sampledPointOnEmitter);
                                Float compositePdfEmitterA_2 = pdfEmitterA_2 * scene.pdfSelectEmitter(emitter);
                                Spectrum Le = emitter->Le(shadowRay);
                                Spectrum contribution = Le * throughput * bsdf_2 * geometryTerm / compositePdfEmitterA_2;
                                L += contribution;
                            }

                        }

                    }

                }
                else
                {
                    L += scene.envMap->Le(thisRay);
                }
                pixelValue += L;

            }
            pixelValue /= sampleCount;
            film.pixels.at(positionInFilm) = pixelValue;

        }
    }
};