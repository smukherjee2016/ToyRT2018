#pragma once

#include "material.hpp"
#include "util/sampler.hpp"

class Phong : public Material {
public:
    Spectrum kS;
    int phongExponent; //Glossiness

    Phong(const Spectrum& _kS, const int _g = 1) {
        kS = _kS;
        phongExponent = _g;
    }

    Vector3 sampleDirection(const Vector3 &wo, const Vector3 &normal, const Point2 pointInPSS) const override {
#if 0
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
#else
        Vector3 reflectedVector = glm::reflect(-wo, normal);
        Point3 pointInCartesian;
        //std::vector<Vector3> arrays;
        //for(int i = 0; i < 10000; i++) {
        Float r1 = rng.generate1DUniform();
        Float r2 = rng.generate1DUniform();

        //Phong importance sampling
        //Theta => [0, 2PI], Phi = [0, PI/2]
        Float theta = 2 * PI * r1;
        Float phi = std::acos(std::pow(r2, 1.0 / (phongExponent + 1.0)));

        pointInCartesian = sphericaltoCartesian(theta, phi);

        //  arrays.emplace_back(Vector3(pointInCartesian.x,pointInCartesian.y,pointInCartesian.z));
        //}
        //saveObj("test.obj", arrays);

        Basis basis;
        basis.makeOrthonormalBasis(reflectedVector); //Sample cone and transform

        Vector3 sampledDirection = glm::normalize(pointInCartesian.x * basis.Cx + pointInCartesian.y * basis.Cy + pointInCartesian.z * basis.Cz);

        if(!areDirectionsSanitized(sampledDirection, wo, normal))
            return Vector3(0.0); //Return black value for things below the horizon
        else
            return sampledDirection;
#endif

    }

    Spectrum brdf(const Vector3& wi, const Vector3& wo, const Vector3& normal) const override {
        if(!areDirectionsSanitized(wi, wo, normal)) {
            return Spectrum(0.0); //Return black value for things below the horizon
        }

        Vector3 reflectedVector = glm::reflect(-wi, normal);
        Float normalizationFactor = (static_cast<Float>(phongExponent) + 2.0) * 0.5 * M_INVPI;

        Float glossiness = normalizationFactor * std::pow(std::max(0.0, glm::dot(wo, reflectedVector)), phongExponent);

        return kS * glossiness;
    }

    Float pdfW(const Vector3 &wi, const Vector3 &wo, const Vector3 &normal) const override {

#if 0
        if(glm::dot(wi, normal) < 0.0 || glm::dot(wo, normal) < 0) {
            return 0.0; //Return black value for things below the horizon
        }

        return M_INVPI * glm::dot(wi, normal); // cos(Phi) / PI = dot(sampledDirection, normal) / PI
#else

        if(!areDirectionsSanitized(wi, wo, normal)) {
            return 0.0; //Return black value for things below the horizon
        }

        Vector3 reflectedVector = glm::reflect(-wi, normal);
        Float cosReflect = std::max(0.0, glm::dot(wo, reflectedVector));
        if(cosReflect <= 0.0) return 0.0;

        Float importanceSamplingPdf = (phongExponent + 1.0) * 0.5 * M_INVPI * std::pow(cosReflect, phongExponent); //(n+1)/2PI
        return importanceSamplingPdf;
#endif
    }

};