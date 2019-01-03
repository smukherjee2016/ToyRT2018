#define TINYOBJLOADER_IMPLEMENTATION
#include <iostream>
#include "common/common.hpp"
#include "camera/pinholecamera.hpp"
#include "integrators/toyIntegrator.hpp"
#include "integrators/ptEmitterv4.hpp"
#include "integrators/ptv4.hpp"
#include "integrators/ptBSDFv3.hpp"
#include "film/film.hpp"
#include "scene/scene.hpp"
#include "util/sampler.hpp"
#include <chrono>

const int sampleCount = 1024;
const int numBounces = 3;

int main(void) {

    int resX = 512;
    int resY = 512;

    //std::cout << rng.generate1DUniform() << std::endl;

    Scene scene;
    scene.makeScene("../scenes/testSphereCboxScene.json");
    //ToyIntegrator toyIntegrator;
    //toyIntegrator.render(pinholeCamera, film, scene, sampleCount);
    //PathTracingEmitterv4 ptIntegrator;
    PathTracingIntegratorv4 ptIntegrator;
    //PathTracingBSDFv3 ptIntegrator;
    auto start = std::chrono::steady_clock::now();
    ptIntegrator.render(scene.camera, scene.film, scene, sampleCount, numBounces);
    auto end = std::chrono::steady_clock::now();

    scene.film->writePixels("Assignment4_normals.pfm");

    std::cout << "Time taken to render a " << resX << " by " << resY << " image using "
        << sampleCount << " samples and " << numBounces << " bounces: "
        << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << " ms.\n";
    return EXIT_SUCCESS;
}