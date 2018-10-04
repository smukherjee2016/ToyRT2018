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
    virtual Spectrum Le(const Ray& incomingRay) const override {
        return Vector3(0.0);
    }

    //IsEmitter
    bool isEmitter() const override {
        return false;
    }

    Sphere(Point3 _center, Float _radius, std::shared_ptr<Material> _mat = nullptr) :
    center(_center), radius(_radius) {
            mat = _mat; //Base class members apparently doesn't work otherwise...
    }

    Point3 center = {0, 0, 0};
    Float radius = 1.0;

};