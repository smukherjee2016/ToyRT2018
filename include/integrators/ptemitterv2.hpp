#pragma once

#include "integrators/integrator.hpp"

typedef std::optional<HitBundle> HitBundleOptional;
typedef std::optional<std::shared_ptr<Object>> ObjectPtrOptional;
typedef std::shared_ptr<Object> ObjectPtr;

class PathTracingEmitterv2 : public Integrator {

public:
    void render(const PinholeCamera& pinholeCamera, Film& film, const Scene& scene, const int sampleCount, const int numBounces) const {
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
                HitBundleOptional hitInformationBundleOptional = traceRayReturnClosestHit(thisRay, scene);

                if(hitInformationBundleOptional) {
                    HitBundle hitInfoBundle_Bounce1 = hitInformationBundleOptional.value();
                    if(hitInfoBundle_Bounce1.closestObject->isEmitter())
                    {
                        //L += hitInfoBundle_Bounce1.closestObject->Le(thisRay);
                        L += Vector3(0.0);
                        continue;
                    }

                    Vector3 sampledNextDirection = hitInfoBundle_Bounce1.closestObject->mat->sampleDirection(-thisRay.d, hitInfoBundle_Bounce1.hitInfo.normal);
                    Spectrum bsdf = hitInfoBundle_Bounce1.closestObject->mat->brdf(sampledNextDirection, -thisRay.d, hitInfoBundle_Bounce1.hitInfo.normal);
                    Float pdfBSDFW_1 = hitInfoBundle_Bounce1.closestObject->mat->pdfW(sampledNextDirection, -thisRay.d, hitInfoBundle_Bounce1.hitInfo.normal);

                    if(pdfBSDFW_1 == 0.0)
                        continue;

                    throughput *= (bsdf / pdfBSDFW_1);

                    Ray ray_2(hitInfoBundle_Bounce1.hitInfo.intersectionPoint, sampledNextDirection);
                    HitBundleOptional hitInformationBundleOptional2 = traceRayReturnClosestHit(ray_2, scene);

                    //Bounce 2
                    if(hitInformationBundleOptional2) {
                        HitBundle hitInfoBundle_Bounce2 = hitInformationBundleOptional2.value();
                        if(hitInfoBundle_Bounce2.closestObject->isEmitter())
                        {
                            L += Vector3(0.0);
                            continue;
                        }

                        //Emitter sampling
                        ObjectPtrOptional emitterOptional = scene.selectRandomEmitter();
                        if(emitterOptional) { //Found an emitter
                            ObjectPtr emitter = emitterOptional.value();

                            Point3 sampledPointOnEmitter = emitter->samplePointOnEmitter();
                            Vector3 outgoingDirection_Emitter = glm::normalize(sampledPointOnEmitter - hitInfoBundle_Bounce2.hitInfo.intersectionPoint);
                            Spectrum bsdf_2 = hitInfoBundle_Bounce2.closestObject->mat->brdf(outgoingDirection_Emitter, -ray_2.d, hitInfoBundle_Bounce2.hitInfo.normal);
                            Float pdfBSDFW_2 = hitInfoBundle_Bounce2.closestObject->mat->pdfW(outgoingDirection_Emitter, -ray_2.d, hitInfoBundle_Bounce2.hitInfo.normal);

                            if(pdfBSDFW_2 == 0.0)
                                continue;

                            Float tMax = glm::length(sampledPointOnEmitter - hitInfoBundle_Bounce2.hitInfo.intersectionPoint) - epsilon;
                            Ray shadowRay(hitInfoBundle_Bounce2.hitInfo.intersectionPoint, outgoingDirection_Emitter, epsilon, tMax);
                            HitBundleOptional shadowRayHitSomething = traceRayReturnClosestHit(shadowRay, scene);
                            if(!shadowRayHitSomething) {
                                Vector3 emitterNormal = emitter->getNormalForEmitter(sampledPointOnEmitter);
                                Float squaredDistance = glm::length2(sampledPointOnEmitter - hitInfoBundle_Bounce2.hitInfo.intersectionPoint);
                                Float geometryTerm = std::max(0.0, glm::dot(-outgoingDirection_Emitter, emitterNormal)) //cos(phi)
                                        * std::max(0.0, glm::dot(outgoingDirection_Emitter, hitInfoBundle_Bounce2.hitInfo.normal)) //cos(theta)
                                        / squaredDistance;

                                Float pdfEmitterA_2 = emitter->pdfEmitterA(sampledPointOnEmitter);
                                Float compositePdfEmitterA_2 = pdfEmitterA_2 * scene.pdfSelectEmitter(emitter);
                                Spectrum Le = emitter->Le(shadowRay);
                                Spectrum contribution = Le * throughput * bsdf_2 * geometryTerm / pdfEmitterA_2;
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