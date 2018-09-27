#pragma once


#include "hitable.hpp"
#include "common/common.hpp"

class Sphere : public Hitable {
public:
    bool didItHitSomething(const Ray& ray) override {
        Float a = glm::dot(ray.d, ray.d);
        Float b = 2.0 * glm::dot(ray.d, (ray.o - center));
        Float c = (glm::dot(ray.o - center, ray.o - center) - radius * radius);
        Float determinant = (b * b) - (4.0 * a * c);
        if(determinant < 0.0)
            return false;
        else if(determinant > 0.0) {
            Float t1 = (-b + determinant) / (2.0 * a);
            Float t2 = (-b - determinant) / (2.0 * a);
            if(t1 < 0.0 && t2 < 0.0) //Both points lie behind camera
                return false;
        }

        return true;
    }

    HitInfo returnClosestHit(const Ray& ray) override {
        Float a = glm::dot(ray.d, ray.d);
        Float b = 2.0 * glm::dot(ray.d, (ray.o - center));
        Float c = (glm::dot(ray.o - center, ray.o - center) - radius * radius);
        Float determinant = (b * b) - (4.0 * a * c);

        HitInfo hitInfo;

        if(determinant < 0.0) {
            hitInfo.tIntersection = 0.0;
            hitInfo.normal = {0.0, 0.0, 0.0};
        }
        else {
            Float t1 = (-b + determinant) / (2.0 * a);
            Float t2 = (-b - determinant) / (2.0 * a);

            if(determinant == 0.0)
                hitInfo.tIntersection = t1;

            if(t1 > 0.0 && t2 > 0.0)
                hitInfo.tIntersection = std::min(t1, t2);
            else if(t1 > 0.0 && t2 < 0.0)
                hitInfo.tIntersection =  t1;
            else if(t1 < 0.0 && t2 > 0.0)
                hitInfo.tIntersection = t2;
        }
        Point3 intersectionPoint = ray.o + hitInfo.tIntersection * ray.d;
        Vector3 uncheckedNormal = glm::normalize( (intersectionPoint - center) / radius );
        hitInfo.normal = glm::dot(uncheckedNormal, ray.d) < 0 ? -uncheckedNormal : uncheckedNormal; //Invert toward incoming ray
        return hitInfo;

    }
    Sphere(Point3 _center, Float _radius) {
        center = _center;
        radius = _radius;
    }
private:
    Point3 center = {0, 0, 0};
    Float radius = 1.0;
};