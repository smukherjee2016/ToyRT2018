#include "integrator.hpp"

class ToyIntegrator : public Integrator {
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

                    for (int k = 1; k <= numBounces; k++) {
                        Ray cameraRay = camera->generateCameraRay(x, y, film);

                        std::optional<HitBundle> hitBundle = scene.traceRayReturnClosestHit(cameraRay);

                        if (hitBundle) {
                            HitBundle cameraRayHitBundle = hitBundle.value();

                            //If hit the emitter, return its Le
                            if (cameraRayHitBundle.closestObject->isEmitter())
                                pixelValue += cameraRayHitBundle.closestObject->emitter->Le(cameraRay);
                            else {
                                //Emitter Sampling
                                std::optional<std::shared_ptr<Emitter>> emitterOptionalBundle = scene.selectRandomEmitter();
                                if (emitterOptionalBundle) {
                                    std::shared_ptr<Emitter> emitter = emitterOptionalBundle.value();
                                    Point3 pointOnLightSource = emitter->samplePointOnEmitter(Sampler());
                                    Vector3 outgoingDirection = glm::normalize(
                                            pointOnLightSource - cameraRayHitBundle.hitInfo.intersectionPoint);
                                    Float pdfEmitterA_EmitterSampling = emitter->pdfSelectPointOnEmitterA(
                                            cameraRayHitBundle.hitInfo.intersectionPoint);

                                    Spectrum brdf = cameraRayHitBundle.closestObject->mat->brdf(outgoingDirection,
                                                                                                -cameraRay.d,
                                                                                                cameraRayHitBundle.hitInfo.normal);
                                    Float pdfBSDF_EmitterSampling = cameraRayHitBundle.closestObject->mat->pdfW(
                                            outgoingDirection, -cameraRay.d,
                                            cameraRayHitBundle.hitInfo.normal);

                                    Float tMax = glm::length(
                                            pointOnLightSource - cameraRayHitBundle.hitInfo.intersectionPoint) -
                                                 epsilon;
                                    Ray nextRay(cameraRayHitBundle.hitInfo.intersectionPoint, outgoingDirection,
                                                epsilon, tMax);
                                    //Ray nextRay(cameraRayHitBundle.hitInfo.intersectionPoint, outgoingDirection);

                                    std::optional<HitBundle> nextRayHitBundle = scene.traceRayReturnClosestHit(nextRay);
                                    if (!nextRayHitBundle) {
                                        //Unoccluded so we can reach light source
                                        Vector3 emitterNormal = emitter->getNormalForEmitter(pointOnLightSource);

                                        Float squaredDistance = glm::length2(
                                                pointOnLightSource - cameraRayHitBundle.hitInfo.intersectionPoint);
                                        Float geometryTerm = std::max(0.0, glm::dot(outgoingDirection,
                                                                                    cameraRayHitBundle.hitInfo.normal)) *
                                                             std::max(0.0, glm::dot(emitterNormal, -outgoingDirection))
                                                             / squaredDistance;

                                        Float pdfBSDFA_EmitterSampling = pdfBSDF_EmitterSampling * std::max(0.0,
                                                                                                            glm::dot(
                                                                                                                    emitterNormal,
                                                                                                                    -outgoingDirection)) /
                                                                         squaredDistance; //Convert to area domain

                                        Float compositeEmitterPdfA_EmitterSampling =
                                                pdfEmitterA_EmitterSampling * scene.pdfSelectEmitter(emitter);

                                        Float misWeight = PowerHeuristic(compositeEmitterPdfA_EmitterSampling,
                                                                         pdfBSDFA_EmitterSampling);
                                        pixelValue += emitter->Le(nextRay) * brdf * geometryTerm * misWeight /
                                                      compositeEmitterPdfA_EmitterSampling;
                                        //pixelValue /= emitterBundle.pdfSelectEmitter;

                                    } else {
                                        pixelValue += Vector3(0.0, 0.0, 0.0); //Light sample occluded so discard it
                                    }

                                } else { //No light sources in scene, so try to sample from envmap or return error
                                    pixelValue += Vector3(0.0);
                                }

                                //BSDF sampling
                                Vector3 outgoingDirection = cameraRayHitBundle.closestObject->mat->sampleDirection(
                                        -cameraRay.d,
                                        cameraRayHitBundle.hitInfo.normal);
                                Spectrum brdf = cameraRayHitBundle.closestObject->mat->brdf(outgoingDirection,
                                                                                            -cameraRay.d,
                                                                                            cameraRayHitBundle.hitInfo.normal);
                                Float pdfBSDF_BSDFSampling = cameraRayHitBundle.closestObject->mat->pdfW(
                                        outgoingDirection,
                                        -cameraRay.d,
                                        cameraRayHitBundle.hitInfo.normal);
                                Ray nextRay(cameraRayHitBundle.hitInfo.intersectionPoint, outgoingDirection);
                                std::optional<HitBundle> nextRayHitBundle = scene.traceRayReturnClosestHit(nextRay);
                                if (nextRayHitBundle) {
                                    HitBundle nextBundle = nextRayHitBundle.value();

                                    //If hit a light source, return its Le
                                    if (nextBundle.closestObject->isEmitter()) {
                                        Float pdfEmitter_BSDFSampling =
                                                nextBundle.closestObject->emitter->pdfSelectPointOnEmitterA(
                                                        nextBundle.hitInfo.intersectionPoint) *
                                                scene.pdfSelectEmitter(nextBundle.closestObject->emitter);

                                        //Convert the SA BSDF pdf into Area domain for MIS calculation
                                        Float squaredDistance = glm::length2(nextBundle.hitInfo.intersectionPoint -
                                                                             cameraRayHitBundle.hitInfo.intersectionPoint);
                                        //Vector3 emitterNormal = nextBundle.closestObject->getNormalForEmitter(nextBundle.hitInfo.intersectionPoint);
                                        Float pdfBSDFA_BSDFSampling = pdfBSDF_BSDFSampling * glm::dot(outgoingDirection,
                                                                                                      cameraRayHitBundle.hitInfo.normal) /
                                                                      squaredDistance;
                                        Float misweight = PowerHeuristic(pdfBSDFA_BSDFSampling,
                                                                         pdfEmitter_BSDFSampling);

                                        pixelValue += nextBundle.closestObject->emitter->Le(nextRay) * brdf *
                                                      glm::dot(outgoingDirection, cameraRayHitBundle.hitInfo.normal) *
                                                      misweight / pdfBSDF_BSDFSampling;
                                    } else {
                                        pixelValue += Vector3(
                                                0.0); //Since this is direct lighting, ignore bounce on other object
                                    }


                                } else {
                                    //Did not hit any light source so zero contribution
                                    //TODO Stop using env map as a special emitter and merge into existing emitter implementation
                                    pixelValue += Vector3(0.0);
                                }

                            }

                        } else { //Did not hit any object so hit environment map
                            pixelValue += scene.envMap->Le(cameraRay);
                        }
                    }

                }
                pixelValue /= (sampleCount);
                film->pixels.at(positionInFilm) = pixelValue;

                if (std::hash<std::thread::id>{}(std::this_thread::get_id()) == 1 && x % film->screenWidth == 0) {
                    std::cout << "Completed "
                              << (static_cast<Float>(positionInFilm) / (film->screenWidth * film->screenHeight)) * 100
                              << " percent.\n";
                }
            }
        });
    }

};