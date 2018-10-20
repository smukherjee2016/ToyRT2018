#include <iostream>
#include "common/common.hpp"
#include "camera/pinholecamera.hpp"
#include "integrators/toyIntegrator.hpp"
#include "integrators/pt.hpp"
#include "film/film.hpp"
#include "scene/scene.hpp"
#include "util/rng.hpp"

const int sampleCount = 16;
const int numBounces = 5;

int main(void) {

    int resX = 512;
    int resY = 512;

    //std::cout << rng.generate1DUniform() << std::endl;

    Film film(M_PI/6.0, 1.0, resX, resY);
    PinholeCamera pinholeCamera(Point3(50,52,295.6), Point3(50,52,295.6) + Point3(0, -0.042612, -1), Point3(0.0, 1.0, 0.0));
    Scene scene;
    scene.makeScene();
    //ToyIntegrator toyIntegrator;
    //toyIntegrator.render(pinholeCamera, film, scene, sampleCount);
    PathTracingIntegrator ptIntegrator;
    ptIntegrator.render(pinholeCamera, film, scene, sampleCount, numBounces);

    film.writePixels("Assignment4_normals.pfm");

    return EXIT_SUCCESS;
}