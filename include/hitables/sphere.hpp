#pragma once


#include "objects/object.hpp"
#include "common/common.hpp"
#include "materials/material.hpp"

inline bool solveQuadraticEquation(const double a, const double b, const double c, double* t1, double* t2)
{
    double discriminant = b * b - 4 * a * c;
    if (discriminant < 0) return false; //No intersection
    else if (discriminant == 0) *t1 = *t2 = - 0.5 * b / a; //Same solution
    else {
        //Numerically more robust method
        double q;
        if(b > 0.0) {
            q = -0.5 * (b + sqrt(discriminant));
        }
        else {
            q = -0.5 * (b - sqrt(discriminant));
        }

        *t1 = q / a;
        *t2 = c / q;
    }
    if (t1 > t2) std::swap(t1, t2);

    return true;
}

class Sphere : public Object {
public:
        std::optional<HitInfo> checkIntersectionAndClosestHit(const Ray& ray) const override{
            glm::dvec3 highPrecisionOrigin = ray.o;
            glm::dvec3 highPrecisionDirection = ray.d;

            double a = glm::dot(highPrecisionDirection, highPrecisionDirection);
            double b = 2.0 * glm::dot(highPrecisionDirection, (highPrecisionOrigin - center));
            double c = (glm::dot(highPrecisionOrigin - center, highPrecisionOrigin - center) - radius * radius);

            HitInfo hitInfo;

            double t1, t2;
            if(!solveQuadraticEquation(a, b, c, &t1, &t2)) //Filters out cases when quadratic equation has no solution
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

            glm::dvec3 intersectionPoint = ray.o + hitInfo.tIntersection * ray.d;
            hitInfo.intersectionPoint = intersectionPoint;
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

    Point3 samplePointOnEmitter() const override {
        Point2 thetaPhi;
        Point3 pointInCartesian;

        //std::vector<Vector3> arrays;
        //for(int i = 0; i < 10000; i++) {
        Float r1 = rng.generate1DUniform();
        Float r2 = rng.generate1DUniform();

        //Uniform weighted sphere sampling
        //Theta => [0, 2PI], Phi = [0, PI/2]
        thetaPhi = uniformSphereSample(r1, r2);

        pointInCartesian = sphericaltoCartesian(thetaPhi.x, thetaPhi.y);


        //    arrays.emplace_back(pointInCartesian);
        //}
        //saveObj("test.obj", arrays);

        //This cartesian point is w.r.t center of the sphere, so return the point in world space
        return (pointInCartesian * radius + static_cast<Point3>(center));

    }

    Float pdfEmitterA(const Point3 &point) const override {
        Float pdfAreaDomain = 1.0 / (4.0 * PI * radius * radius); // 1/4PI
        return pdfAreaDomain;
    }

    Vector3 getNormalForEmitter(const Point3& point) const override {
        return glm::normalize((point - center) / radius);
    }

    Sphere(Point3 _center, Float _radius, std::shared_ptr<Material> _mat = nullptr, Spectrum _Le = Vector3(0.0)) :
    center(_center), radius(_radius) {
            mat = _mat; //Base class members apparently doesn't work otherwise...
            //LeIntensity = _Le / (4 * PI * _radius * _radius);
            LeIntensity = _Le;
    }

    glm::dvec3 center = {0, 0, 0};
    double radius = 1.0;
    Spectrum LeIntensity;

};