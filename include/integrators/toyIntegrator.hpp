#include "integrator.hpp"

class ToyIntegrator : public Integrator {
public:
    void render(const PinholeCamera& pinholeCamera, Film& film, const Scene& scene) const override {
        for(int y = 0; y < film.screenHeight; y++) {
            for(int x = 0; x < film.screenWidth; x++) {
                int positionInFilm = y * film.screenWidth + x;

                Ray cameraRay = pinholeCamera.generateCameraRay(x, y, film);
                HitInfo closestHit;
                //film.pixels.at(positionInFilm) = Vector3(0.0, 0.1, 0.5);
                //film.pixels.at(positionInFilm) = cameraRay.d;
                for(auto sphere: scene.spheres) {
                    if(sphere.didItHitSomething(cameraRay)) {
                        closestHit = sphere.returnClosestHit(cameraRay);
                    }
                    else
                    {
                        closestHit = {0.0, {0.0,0.0,0.0}};
                    }
                }
                //film.pixels.at(positionInFilm) = closestHit.normal;
                film.pixels.at(positionInFilm) = Vector3(closestHit.tIntersection);
            }
        }
    }
};