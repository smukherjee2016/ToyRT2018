#include <iostream>
#include "common/common.hpp"
#include "camera/pinholecamera.hpp"
#include "integrators/toyIntegrator.hpp"
#include "film/film.hpp"
#include "scene/scene.hpp"
#include "util/rng.hpp"

int main(void) {

    int resX = 1024;
    int resY = 576;

    std::cout << rng.generate1DUniform() << std::endl;

    Film film(M_PI/4.0, 1.0, resX, resY);
    PinholeCamera pinholeCamera(Point3(0.0, 0.0, 1.0), Point3(0.0,0.0,0.0), Point3(0.0, 1.0, 0.0));
    Scene scene;
    scene.makeScene();
    ToyIntegrator toyIntegrator;
    toyIntegrator.render(pinholeCamera, film, scene);
    film.writePixels("Assignment4_normals.pfm");

    return EXIT_SUCCESS;
}