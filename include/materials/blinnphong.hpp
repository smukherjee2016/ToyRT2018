#pragma once

#include "material.hpp"
#include "util/sampler.hpp"

class BlinnPhong : public Material {
public:
    Spectrum kS;
    int g; //Glossiness

    BlinnPhong(const Spectrum& _kS, const int _g = 1) {
        kS = _kS;
        g = _g;
    }

    Vector3 sampleDirection(const Vector3&, const Vector3& normal) const {

        Point3 pointInCartesian;
        //std::vector<Vector3> arrays;
        //for(int i = 0; i < 10000; i++) {
        Float r1 = rng.generate1DUniform();
        Float r2 = rng.generate1DUniform();

        //Cosine weighted hemisphere sampling
        //Theta => [0, 2PI], Phi = [0, PI/2]
        Float theta = 2 * PI * r1;
        Float phi = std::acos(std::sqrt(r2));

        pointInCartesian = sphericaltoCartesian(theta, phi);

        //  arrays.emplace_back(Vector3(pointInCartesian.x,pointInCartesian.y,pointInCartesian.z));
        //}
        //saveObj("test.obj", arrays);

        Basis basis;
        basis.makeOrthonormalBasis(normal);

        return glm::normalize(pointInCartesian.x * basis.Cx + pointInCartesian.y * basis.Cy + pointInCartesian.z * basis.Cz);

    }

    Spectrum brdf(const Vector3& wi, const Vector3& wo, const Vector3& normal) const {
        if(glm::dot(wi, normal) < 0.0 || glm::dot(wo, normal) < 0) {
            return Spectrum(0.0); //Return black value for things below the horizon
        }

        Vector3 h = glm::normalize(wi + wo);
        Float glossiness = std::pow(std::max(0.0, glm::dot(normal, h)), g);

        return kS * glossiness;
    }

    Float pdfW(const Vector3 &wi, const Vector3 &wo, const Vector3 &normal) const {
        if(glm::dot(wi, normal) < 0.0 || glm::dot(wo, normal) < 0) {
            return 0.0; //Return black value for things below the horizon
        }

        return M_INVPI * glm::dot(wi, normal); // cos(Phi) / PI = dot(sampledDirection, normal) / PI
    }

};