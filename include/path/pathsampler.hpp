#pragma once


#include "path.hpp"

//Forward declaration of traceRayReturnClosestHit from integrator.hpp
std::optional<HitBundle> traceRayReturnClosestHit(const Ray &ray);

#include "integrators/integrator.hpp"


class PathSampler {

public:
    //TODO The path object can be large and memory copies can be expensive... Look at how to prevent copies if they happen
    Path generatePath(Scene& scene, const Ray& cameraRay, const int numBounces) {
        Path path{};
        Ray currentRay = cameraRay;

        //Add the camera vertex, a tiny tiny transparent sphere
        HitBundle cameraPointBundle{};
        cameraPointBundle.hitInfo.intersectionPoint = cameraRay.o;
        cameraPointBundle.closestObject = std::make_unique<Sphere>(cameraRay.o, epsilon , std::make_shared<TransparentMaterial>());
        //Fill in normal, geoTerm and pdfA later so they cancel out

        Vertex cameraVertex{};
        cameraVertex.hitPointAndMaterial = cameraPointBundle;
        cameraVertex.pdfBSDFW = 1.0; //Probability of sampling this vertex in SA domain is always 1
        cameraVertex.bsdf_xi_xiplus1 = Spectrum(1.0); //Pass all the light through
        //Set pdfA and Geoterm later
        cameraVertex.pdfBSDFA = 1.0;
        cameraVertex.G_xi_xiplus1 = 1.0;

        //Set camera  vertex to have SENSOR type
        cameraVertex.vertexType = SENSOR;


        path.vertices.emplace_back(cameraVertex);


        //Accumulate path, assume first vertex on the path is the first hitpoint, not the camera
        for(int k = 1; k <= numBounces; k++) {
            std::optional<HitBundle> didCurrentRayHitObject = scene.traceRayReturnClosestHit(currentRay);
            if(didCurrentRayHitObject) {
                HitBundle currentHitBundle = didCurrentRayHitObject.value();

                //Special processing for the sensor vertex: Set normal to be the same direction as towards
                //the first hitPoint so that cos(theta) = 1 and cos(phi)/d^2 and the geoTerm cancel out
                if(k == 1 && path.vertices.at(0).vertexType == SENSOR) {
                    Point3 cameraPosition = path.vertices.at(0).hitPointAndMaterial.hitInfo.intersectionPoint;
                    Point3 firstHitPointPosition = currentHitBundle.hitInfo.intersectionPoint;

                    path.vertices.at(0).hitPointAndMaterial.hitInfo.normal
                            = glm::normalize(firstHitPointPosition - cameraPosition);


                }

                //If hit an emitter, store only the hitBundle object and terminate the path
                if(currentHitBundle.closestObject->isEmitter()) {
                    Vertex emitterVertex{};
                    emitterVertex.vertexType = EMITTER;

                    emitterVertex.hitPointAndMaterial = currentHitBundle;
                    emitterVertex.bsdf_xi_xiplus1 = Vector3(1.0);
                    emitterVertex.pdfBSDFW = 1.0;

                    //Keep geometry term and area pdf to 1 if hit emitter
                    emitterVertex.pdfBSDFA = 1.0;
                    emitterVertex.G_xi_xiplus1 = 1.0;

                    path.vertices.emplace_back(emitterVertex);
                    break;
                }

                //Calculate shading point values
                Vector3 hitPointNormal = currentHitBundle.hitInfo.normal;
                Vector3 hitPoint = currentHitBundle.hitInfo.intersectionPoint;

                //Sample next direction alongwith the pdfs
                //TODO Handle zero samples properly return Sample object with info instead of Vector
                Vector3 sampledNextBSDFDirection = currentHitBundle.closestObject->mat->sampleDirection(-currentRay.d, hitPointNormal);
                Spectrum bsdf = currentHitBundle.closestObject->mat->brdf(sampledNextBSDFDirection, -currentRay.d, hitPointNormal);
                Float pdfW = currentHitBundle.closestObject->mat->pdfW(sampledNextBSDFDirection, -currentRay.d, hitPointNormal);


                Vertex currentVertex{};
                //Store the hitBundles etc. in the path
                currentVertex.hitPointAndMaterial = currentHitBundle;
                currentVertex.bsdf_xi_xiplus1 = bsdf;
                currentVertex.pdfBSDFW = pdfW;

                //Keep geometry term, throughput, and area-domain pdf to 1. Will be filled after whole path is constructed
                currentVertex.pdfBSDFA = 1.0;
                currentVertex.G_xi_xiplus1 = 1.0;

                path.vertices.emplace_back(currentVertex);
                //Generate next ray and make it current
                Ray nextRay(hitPoint, sampledNextBSDFDirection);
                currentRay = nextRay;

                //There is no next sample and direction, so stop making the path here
                if(pdfW == 0.0)
                    break;
            }
            else
                break; //Don't do any (more) bounces if didn't hit anything
        }

        int numVertices = path.vertices.size();
        //Process the path and fill in geometry term, throughput and area-domain pdf
        for(auto vertexIndex = 0; vertexIndex < numVertices - 1; vertexIndex++) {
            //Extract current and previous vertices for calculation
            HitBundle thisVertex = path.vertices.at(vertexIndex).hitPointAndMaterial;
            HitBundle nextVertex = path.vertices.at(vertexIndex + 1).hitPointAndMaterial;

            Float squaredDistance = glm::length2(nextVertex.hitInfo.intersectionPoint - thisVertex.hitInfo.intersectionPoint);
            Vector3 directionThisToNext = glm::normalize(nextVertex.hitInfo.intersectionPoint - thisVertex.hitInfo.intersectionPoint);
            Float geometryTerm = std::max(0.0, glm::dot(directionThisToNext, thisVertex.hitInfo.normal)) //cos(Theta)_(i-1)
                                 * std::max(0.0, glm::dot(-directionThisToNext, nextVertex.hitInfo.normal)) //cos(Phi)_i
                                 / squaredDistance;

            Float pdfWAConversionFactor_XiPlus1GivenXi = std::max(0.0, glm::dot(-directionThisToNext, nextVertex.hitInfo.normal)) //cos(Phi)_i
                                                         / squaredDistance;

            Float pdfA_XiPlus1GivenXi = path.vertices.at(vertexIndex).pdfBSDFW * pdfWAConversionFactor_XiPlus1GivenXi;

            //Store geometry terms and area domain BSDFs
            path.vertices.at(vertexIndex).G_xi_xiplus1 = geometryTerm;
            path.vertices.at(vertexIndex).pdfBSDFA = pdfA_XiPlus1GivenXi;

        }

        return path;
    }
};