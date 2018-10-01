#pragma once

#include "hitable.hpp"
/*
 * A plane is defined by a point and a normal.
 */
class Plane : public Hitable {

public:
    Point3 distanceFromWorldOrigin;
    Vector3 normal;


    Plane(const Point3& _distanceFromWorldOrigin, const Vector3& _normal) {
        distanceFromWorldOrigin = _distanceFromWorldOrigin;
        normal = _normal;
    }

    bool didItHitSomething(const Ray& ray) const  {
        //TODO: Should we modify the actual normal here and below?
        Vector3 tempNormal = normal; //Invert direction toward ray
        const Float epsilon = 1e-6;
        Float denominator = glm::dot(ray.d, tempNormal);
        if(denominator < epsilon )
            return false;

        Float numerator = glm::dot((distanceFromWorldOrigin - ray.o), tempNormal);
        Float tSolution = numerator / denominator;
        if(tSolution <= 0.0)
            return false;

        return true;
    }
    HitInfo returnClosestHit(const Ray& ray) const  {
        Vector3 tempNormal = normal; //Invert direction toward ray

        Float denominator = glm::dot(ray.d, tempNormal);
        Float numerator = glm::dot((distanceFromWorldOrigin - ray.o), tempNormal);

        HitInfo hitInfo;
        hitInfo.tIntersection = numerator / denominator;
        hitInfo.normal = tempNormal;

        return hitInfo;
    }

};