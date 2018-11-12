#include <iostream>
#include "common/common.hpp"
#include "camera/pinholecamera.hpp"
#include "integrators/toyIntegrator.hpp"
#include "integrators/pt.hpp"
#include "integrators/ptemitter.hpp"
#include "integrators/ptemitterv2.hpp"
#include "integrators/ptemitterv4.hpp"
#include "integrators/ptBSDFv3.hpp"
#include "film/film.hpp"
#include "scene/scene.hpp"
#include "util/rng.hpp"
#include <chrono>

const int sampleCount = 16;
const int numBounces = 5;

int main(void) {

    int resX = 512;
    int resY = 512;

    //std::cout << rng.generate1DUniform() << std::endl;

    Film film(PI/6.0, 1.0, resX, resY);
    PinholeCamera pinholeCamera(Point3(50,52,295.6), Point3(50, 51.9574, 294.601), Point3(0.0, 0.999093, -0.0425734));
    Scene scene;
    scene.makeScene();
    //ToyIntegrator toyIntegrator;
    //toyIntegrator.render(pinholeCamera, film, scene, sampleCount);
    //PathTracingIntegrator ptIntegrator;
    //ptIntegrator.render(pinholeCamera, film, scene, sampleCount, numBounces);
    //PathTracingEmitterv2 ptIntegrator;
    PathTracingEmitterv4 ptIntegrator;
    //PathTracingBSDFv3 ptIntegrator;
    auto start = std::chrono::steady_clock::now();
    ptIntegrator.render(pinholeCamera, film, scene, sampleCount, numBounces);
    auto end = std::chrono::steady_clock::now();

    film.writePixels("Assignment4_normals.pfm");

    std::cout << "Time taken to render a " << resX << " by " << resY << " image using "
        << sampleCount << " samples and " << numBounces << " bounces: "
        << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << " ms.\n";
    return EXIT_SUCCESS;
}