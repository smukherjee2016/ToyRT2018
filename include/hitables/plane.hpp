#pragma once

#include "hitable.hpp"
/*
 * A plane is defined by a point and a normal.
 */
class Plane : public Hitable {
    Point3 distanceFromWorldOrigin;
    Vector3 normal;

    Plane(const Point3& _distanceFromWorldOrigin, const Vector3& _normal) {
        distanceFromWorldOrigin = _distanceFromWorldOrigin;
        normal = _normal;
    }

    bool didItHitSomething(const Ray& ray) {
        const Float epsilon = 1e-6;
        Float denominator = glm::dot(ray.d, normal);
        if(denominator < epsilon )
            return false;

        Float numerator = glm::dot((distanceFromWorldOrigin - ray.o), normal);
        Float tSolution = numerator / denominator;
        if(tSolution <= 0.0)
            return false;
    }
    HitInfo returnClosestHit(const Ray& ray) {
        Float denominator = glm::dot(ray.d, normal);
        Float numerator = glm::dot((distanceFromWorldOrigin - ray.o), normal);

        HitInfo hitInfo;
        hitInfo.tIntersection = numerator / denominator;
        hitInfo.normal = normal;
    }

};