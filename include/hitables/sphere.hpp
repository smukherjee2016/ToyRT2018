#pragma once


#include "hitable.hpp"
#include "common/common.hpp"
#include "materials/lambertUniform.hpp"

class Sphere : public Hitable {
public:

    std::optional<HitInfo> checkIntersectionAndClosestHit(const Ray& ray) const override {
        Float a = glm::dot(ray.d, ray.d);
        Float b = 2.0 * glm::dot(ray.d, (ray.o - center));
        Float c = (glm::dot(ray.o - center, ray.o - center) - radius * radius);
        Float determinant = (b * b) - (4.0 * a * c);

        if(determinant < 0.0)
            return std::nullopt;

        HitInfo hitInfo;

        Float t1 = (-b + determinant) / (2.0 * a);
        Float t2 = (-b - determinant) / (2.0 * a);

        if(t1 < 0.0 && t2 < 0.0)
            return std::nullopt;

        if(determinant == 0.0)
            hitInfo.tIntersection = t1;

        if(t1 > 0.0 && t2 > 0.0)
            hitInfo.tIntersection = std::min(t1, t2);
        else if(t1 > 0.0 && t2 < 0.0)
            hitInfo.tIntersection =  t1;
        else if(t1 < 0.0 && t2 > 0.0)
            hitInfo.tIntersection = t2;

        if(hitInfo.tIntersection < ray.tmin || hitInfo.tIntersection > ray.tmax)
            return std::nullopt;

        Point3 intersectionPoint = ray.o + hitInfo.tIntersection * ray.d;
        hitInfo.normal = glm::normalize( (intersectionPoint - center) / radius );

        return {hitInfo};

    }
    Sphere(Point3 _center, Float _radius, std::shared_ptr<Material> _mat = nullptr) {
        center = _center;
        radius = _radius;
        mat = _mat;
    }

    Point3 center = {0, 0, 0};
    Float radius = 1.0;

};