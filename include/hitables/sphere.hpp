#pragma once


#include "objects/object.hpp"
#include "common/common.hpp"
#include "materials/lambertUniform.hpp"

inline bool solveQuadraticEquation(const Float &a, const Float &b, const Float &c, Float &t1, Float &t2)
{
    Float discriminant = b * b - 4 * a * c;
    if (discriminant < 0) return false; //No intersection
    else if (discriminant == 0) t1 = t2 = - 0.5 * b / a; //Same solution
    else {
        //Numerically more robust method
        Float q;
        if(b > 0.0) {
            q = -0.5 * (b + sqrt(discriminant));
        }
        else {
            q = -0.5 * (b - sqrt(discriminant));
        }

        t1 = q / a;
        t2 = c / q;
    }
    if (t1 > t2) std::swap(t1, t2);

    return true;
}

class Sphere : public Object {
public:
        std::optional<HitInfo> checkIntersectionAndClosestHit(const Ray& ray) const override{
        Float a = glm::dot(ray.d, ray.d);
        Float b = 2.0 * glm::dot(ray.d, (ray.o - center));
        Float c = (glm::dot(ray.o - center, ray.o - center) - radius * radius);

        HitInfo hitInfo;

        Float t1, t2;
        if(!solveQuadraticEquation(a, b, c, t1, t2)) //Filters out cases when quadratic equation has no solution
            return std::nullopt;

        if(t1 > t2) std::swap(t1, t2); //Keep the least value in t1 with intention of returning it
        if(t1 < 0.0) { //If t1 is not positive, try to use t2
            t1 = t2;
            if(t1 < 0.0) { //If t2 (put into t1) is also not positive, no hope
                return std::nullopt;
            }
        }

        hitInfo.tIntersection = t1; //The last bastion

        if(hitInfo.tIntersection < ray.tmin || hitInfo.tIntersection > ray.tmax) { //Prevent self-intersection
          return std::nullopt;
        }

        Point3 intersectionPoint = ray.o + hitInfo.tIntersection * ray.d;
        hitInfo.normal = (intersectionPoint - center) / radius;

        return {hitInfo};

    }

    //Emitter
    Spectrum Le(const Ray& incomingRay) const override {
        return LeIntensity;
    }

    //IsEmitter
    bool isEmitter() const override {
        if(LeIntensity == Vector3(0.0))
            return false;
        else
            return true;
    }

    Point3 samplePointOnEmitter(const Vector3& wo, const Vector3& normal) const override {
        Float x,y,z;
        //Vector3 newNormal(1.0, 0.0, 0.0);
        //std::vector<Vector3> arrays;
        //for(int i = 0; i < 10000; i++) {
        Float r1 = rng.generate1DUniform();
        Float r2 = rng.generate1DUniform();

        //Uniform weighted sphere sampling
        //Theta => [0, 2PI], Phi = [0, PI/2]
        Point2 thetaPhi = uniformSphereSample(r1, r2);

        Point3 pointInCartesian = sphericaltoCartesian(thetaPhi.x, thetaPhi.y);


        //    arrays.emplace_back(Vector3(x,y,z));
        //}
        //saveObj("test.obj", arrays);

        Basis basis;
        basis.makeOrthonormalBasis(normal);

        return glm::normalize(pointInCartesian.x * basis.Cx + pointInCartesian.y * basis.Cy + pointInCartesian.z * basis.Cz);

    }

    Float pdfEmitter(const Vector3& wi, const Vector3& wo, const Vector3& normal) const override {
        return 0.25 * M_INVPI; // 1/4PI
    }

    Sphere(Point3 _center, Float _radius, std::shared_ptr<Material> _mat = nullptr, Spectrum _Le = Vector3(0.0)) :
    center(_center), radius(_radius), LeIntensity(_Le) {
            mat = _mat; //Base class members apparently doesn't work otherwise...
    }

    Point3 center = {0, 0, 0};
    Float radius = 1.0;
    Spectrum LeIntensity;

};