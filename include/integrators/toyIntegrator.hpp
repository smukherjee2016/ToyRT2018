#include "integrator.hpp"

const int numBounces = 1;

class ToyIntegrator : public Integrator {
public:
    void render(const PinholeCamera& pinholeCamera, Film& film, const Scene& scene, const int sampleCount) const override {
//#pragma omp parallel for schedule(dynamic, 1)
        for(int i = 0; i < film.screenHeight * film.screenWidth; i++) {
            //int positionInFilm = 286 * film.screenWidth + 637;

            int positionInFilm = i;
            int x = positionInFilm % film.screenWidth;
            int y = positionInFilm / film.screenWidth;
            //int positionInFilm = y * film.screenWidth + x;
            //std::cout << x << "  " << y << std::endl;
            Spectrum pixelValue{};
            for (int j = 0; j < sampleCount; j++) {

                for(int k = 0; k < numBounces; k++) {
                    Ray cameraRay = pinholeCamera.generateCameraRay(x, y, film);

                    std::optional<HitBundle> hitBundle = traceRayReturnClosestHit(cameraRay, scene);

                    if (hitBundle) {
                        HitBundle validHitBundle = hitBundle.value();
                        Vector3 outgoingDirection = validHitBundle.closestObject->mat->sampleDirection(-cameraRay.d,
                                                                                                       validHitBundle.hitInfo.normal);
                        Spectrum brdf = validHitBundle.closestObject->mat->brdf(outgoingDirection, -cameraRay.d,
                                                                                validHitBundle.hitInfo.normal);
                        Float pdf = validHitBundle.closestObject->mat->pdf(outgoingDirection, -cameraRay.d,
                                                                           validHitBundle.hitInfo.normal);
                        Point3 hitPoint = cameraRay.o + validHitBundle.hitInfo.tIntersection * cameraRay.d;
                        //pixelValue += Vector3(glm::length(hitPoint));
                        //pixelValue += validHitBundle.hitInfo.debug_position;
                        //pixelValue += validHitBundle.hitInfo.normal;
                        //pixelValue += Vector3(glm::length(validHitBundle.hitInfo.debug_position));
                        //continue;
                        Ray nextRay(hitPoint, outgoingDirection);
                        std::optional<HitBundle> nextRayHitBundle = traceRayReturnClosestHit(nextRay, scene);
                        if (nextRayHitBundle) {
                            HitBundle nextBundle = nextRayHitBundle.value();
                            //std::cout << nextBundle.hitInfo.tIntersection;
                            pixelValue += Vector3(0.0);

                        } else {
                            pixelValue += scene.envMap->Le(nextRay) * brdf
                                           * glm::dot(outgoingDirection, validHitBundle.hitInfo.normal) / pdf;
                        }
                        //pixelValue = outgoingDirection;
                        //pixelValue = Vector3(closestHit.tIntersection * glm::length(glm::normalize(cameraRay.d)));
                    } else {
                        pixelValue += scene.envMap->Le(cameraRay);
                    }
                }

            }
            pixelValue /= sampleCount;
            film.pixels.at(positionInFilm) = pixelValue;
        }
    }

};