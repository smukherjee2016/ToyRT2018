#include <iostream>
#include "common/common.hpp"
#include "camera/pinholecamera.hpp"
#include "integrators/toyIntegrator.hpp"
#include "film/film.hpp"
#include "scene/scene.hpp"

int main(void) {

    int resX = 640;
    int resY = 360;

    Film film(M_PI/3.0, 1.0, resX, resY);
    PinholeCamera pinholeCamera(Vector3(0.0), Vector3(0.0, 0.0, -1.0), Vector3(0.0, 1.0, 0.0));
    Scene scene;
    scene.makeScene();
    ToyIntegrator toyIntegrator;
    toyIntegrator.render(pinholeCamera, film);
    film.writePixels("Assignment2.pfm");

    return EXIT_SUCCESS;
}