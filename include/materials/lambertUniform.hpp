#pragma once

#include "material.hpp"
#include "util/rng.hpp"

class LambertUniform : public Material {
public:
    Spectrum kD;

    LambertUniform(const Spectrum& _kD) {
        kD = _kD;
    }

    Vector3 sampleDirection(const Vector3& wo, const Vector3& normal) const {

        Float x,y,z;
        //Vector3 newNormal(1.0, 0.0, 0.0);
        //std::vector<Vector3> arrays;
        //for(int i = 0; i < 10000; i++) {
            Float r1 = rng.generate1DUniform();
            Float r2 = rng.generate1DUniform();

            //Uniform weighted hemisphere sampling
            //Theta => [0, 2PI], Phi = [0, PI/2]
            Float theta = 2 * M_PI * r1;
            Float phi = std::acos(r2);

            x = std::cos(theta) * std::sin(phi);
            y = std::cos(phi);
            z = std::sin(theta) * std::sin(phi);
        //    arrays.emplace_back(Vector3(x,y,z));
        //}
        //saveObj("test.obj", arrays);

        Basis basis;
        basis.makeOrthonormalBasis(normal);

        return glm::normalize(x * basis.Cx + y * basis.Cy + z * basis.Cz);

    }

    Spectrum brdf(const Vector3& wi, const Vector3& wo, const Vector3& normal) const {
        if(glm::dot(wi, normal) < 0.0 || glm::dot(wo, normal) < 0) {
            return Spectrum(0.0); //Return black value for things below the horizon
        }

        return kD * M_INVPI;
    }

    virtual Float pdf(const Vector3& wi, const Vector3& wo, const Vector3& normal) const {
        if(glm::dot(wi, normal) < 0.0 || glm::dot(wo, normal) < 0) {
            return 0.0; //Return black value for things below the horizon
        }

        return M_INVPI / 2.0; // 1/2PI
    }

};