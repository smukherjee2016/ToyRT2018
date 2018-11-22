#pragma once

#include "objects/object.hpp"
/*
 * A plane is defined by a point and a normal.
 */
class Plane : public Object {

public:
    Point3 distanceFromWorldOrigin;
    Vector3 normal;


    Plane(const Point3& _distanceFromWorldOrigin, const Vector3& _normal) :
    distanceFromWorldOrigin(_distanceFromWorldOrigin), normal(_normal) {}


    std::optional<HitInfo> checkIntersectionAndClosestHit(const Ray& ray) const override {
        Vector3 tempNormal = normal; //Invert direction toward ray

        const Float epsilon = 1e-5;
        Float denominator = glm::dot(ray.d, tempNormal);
        Float numerator = glm::dot((distanceFromWorldOrigin - ray.o), tempNormal);

        if(denominator < epsilon)
            return std::nullopt;

        Float tSolution = numerator / denominator;

        if(tSolution < 0.0 || tSolution < ray.tmin || tSolution > ray.tmax)
            return std::nullopt;

        HitInfo hitInfo;
        hitInfo.tIntersection = numerator / denominator;
        hitInfo.normal = tempNormal;

        return {hitInfo};
    }

    //IsEmitter
    bool isEmitter() const override {
        return false;
    }

};