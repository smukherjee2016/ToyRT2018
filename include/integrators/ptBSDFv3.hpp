#pragma once

#include <thread>
#include "integrator.hpp"

class PathTracingBSDFv3 : public Integrator {
public:
    void render(std::shared_ptr<Camera> camera, std::shared_ptr<Film> film, Scene &scene, const int sampleCount,
                const int numBounces = 2) const override {
        unsigned int numThreads = std::thread::hardware_concurrency() - 1;
#pragma omp parallel for schedule(dynamic, 1) num_threads(numThreads)
        for (int i = 0; i < film->screenHeight * film->screenWidth; i++) {

            int positionInFilm = i;
            int x = positionInFilm % film->screenWidth;
            int y = positionInFilm / film->screenWidth;
            //int positionInFilm = y * film->screenWidth + x;

            Spectrum pixelValue{};
            for (int j = 0; j < sampleCount; j++) {

                Ray cameraRay = camera->generateCameraRay(x, y, film);
                Ray currentRay = cameraRay;
                Spectrum L(0.0);

                PathSampler pathSampler{};
                Path currentSampleBSDFPath = pathSampler.generatePath(scene, cameraRay, numBounces);

                int numVertices = currentSampleBSDFPath.vertices.size();
                //Process the path and fill in geometry term, throughput and area-domain pdf
                for(auto vertexIndex = 0; vertexIndex < numVertices - 1; vertexIndex++) {
                    //Extract current and previous vertices for calculation
                    HitBundle thisVertex = currentSampleBSDFPath.vertices.at(vertexIndex).hitPointAndMaterial;
                    HitBundle nextVertex = currentSampleBSDFPath.vertices.at(vertexIndex + 1).hitPointAndMaterial;

                    Float squaredDistance = glm::length2(nextVertex.hitInfo.intersectionPoint - thisVertex.hitInfo.intersectionPoint);
                    Vector3 directionThisToNext = glm::normalize(nextVertex.hitInfo.intersectionPoint - thisVertex.hitInfo.intersectionPoint);
                    Float geometryTerm = std::max(0.0, glm::dot(directionThisToNext, thisVertex.hitInfo.normal)) //cos(Theta)_(i-1)
                                         * std::max(0.0, glm::dot(-directionThisToNext, nextVertex.hitInfo.normal)) //cos(Phi)_i
                                         / squaredDistance;

                    Float pdfWAConversionFactor_XiPlus1GivenXi = std::max(0.0, glm::dot(-directionThisToNext, nextVertex.hitInfo.normal)) //cos(Phi)_i
                                                                 / squaredDistance;

                    Float pdfA_XiPlus1GivenXi = currentSampleBSDFPath.vertices.at(vertexIndex).pdfBSDFW * pdfWAConversionFactor_XiPlus1GivenXi;

                    //Store geometry terms and area domain BSDFs
                    currentSampleBSDFPath.vertices.at(vertexIndex).G_xi_xiplus1 = geometryTerm;
                    currentSampleBSDFPath.vertices.at(vertexIndex).pdfBSDFA = pdfA_XiPlus1GivenXi;

                }

                //If final vertex is on an emitter, add contribution : BSDF sampling
                if(numVertices >= 1 && currentSampleBSDFPath.vertices.at(numVertices - 1).vertexType == EMITTER) {
                    if (numVertices == 1) { //Direct hit emitter
                        L = currentSampleBSDFPath.vertices.at(numVertices - 1).hitPointAndMaterial.closestObject->emitter->Le(
                                cameraRay);
                    } else {
                        //Reconstruct final shot ray before hitting the emitter
                        Ray finalBounceRay{};
                        finalBounceRay.o = currentSampleBSDFPath.vertices.at(
                                numVertices - 2).hitPointAndMaterial.hitInfo.intersectionPoint;
                        finalBounceRay.d = glm::normalize(currentSampleBSDFPath.vertices.at(
                                numVertices - 1).hitPointAndMaterial.hitInfo.intersectionPoint
                                                          - currentSampleBSDFPath.vertices.at(
                                numVertices - 2).hitPointAndMaterial.hitInfo.intersectionPoint);

                        //Find Le in the given direction of final shot ray
                        L = currentSampleBSDFPath.vertices.at(numVertices - 1).hitPointAndMaterial.closestObject->emitter->Le(
                                finalBounceRay);
                        //Calculate light transported along this given path to the camera
                        for (int vertexIndex = numVertices - 1; vertexIndex >= 1; vertexIndex--) {
                            //Visibility term implicitly 1 along this path
                            Float geometryTerm = currentSampleBSDFPath.vertices.at(vertexIndex).G_xi_xiplus1;
                            Float pdfBSDFA = currentSampleBSDFPath.vertices.at(vertexIndex).pdfBSDFA;
                            Spectrum bsdf = currentSampleBSDFPath.vertices.at(vertexIndex).bsdf_xi_xiplus1;

                            Spectrum attenuation = bsdf * geometryTerm / pdfBSDFA;
                            L *= attenuation;

                        }
                    }
                }
                pixelValue += L; //Add sample contribution
            }
            pixelValue /= sampleCount; //Average MC estimation
            film->pixels.at(positionInFilm) = pixelValue; //Write to film

        auto tid = omp_get_thread_num();
        if(tid == 0 && x % film->screenWidth == 0)
            std::cout << "Completed " << (static_cast<Float>(positionInFilm) / (film->screenWidth * film->screenHeight)) * 100 << " percent.\n";
    }



}

};