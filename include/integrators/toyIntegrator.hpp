#include "integrator.hpp"

class ToyIntegrator : public Integrator {
public:
    void render(const PinholeCamera& pinholeCamera, Film& film) const override {
        for(int y = 0; y < film.screenHeight; y++) {
            for(int x = 0; x < film.screenWidth; x++) {
                int positionInFilm = y * film.screenWidth + x;

                Ray cameraRay = pinholeCamera.generateCameraRay(x, y, film);

                //film.pixels.at(positionInFilm) = Vector3(0.0, 0.1, 0.5);
                film.pixels.at(positionInFilm) = cameraRay.d;
            }
        }
    }
};