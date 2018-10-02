#pragma once


#include "hitable.hpp"
#include "common/common.hpp"
#include "materials/lambertUniform.hpp"

inline bool solveQuadratic(const float &a, const float &b, const float &c, float &x0, float &x1)
{
    float discr = b * b - 4 * a * c;
    if (discr < 0) return false;
    else if (discr == 0) x0 = x1 = - 0.5 * b / a;
    else {
        float q = (b > 0) ?
                  -0.5 * (b + sqrt(discr)) :
                  -0.5 * (b - sqrt(discr));
        x0 = q / a;
        x1 = c / q;
    }
    if (x0 > x1) std::swap(x0, x1);

    return true;
}



class Sphere : public Hitable {
public:

    /// TODO:: remove -1
    float intersect(const Ray &ray) const
    {
        float t0, t1; // solutions for t if the ray intersects
        Vector3 L = ray.o - center;
        float a = glm::dot(ray.d, ray.d);
        float b = 2 * glm::dot(ray.d,L);
        float c = glm::dot(L, L) - radius * radius;
        if (!solveQuadratic(a, b, c, t0, t1)) return -1;
        if (t0 > t1) std::swap(t0, t1);

        if (t0 < 0) {
            t0 = t1; // if t0 is negative, let's use t1 instead
            if (t0 < 0) return -1; // both t0 and t1 are negative
        }

        return t0;
    }


    std::optional<HitInfo> checkIntersectionAndClosestHit(const Ray& ray) const override {
        float t = intersect(ray);
        if (t == -1) return std::nullopt;
        HitInfo hitInfo;
        hitInfo.tIntersection = t;

        if(hitInfo.tIntersection < ray.tmin || hitInfo.tIntersection > ray.tmax) {
            //  __debugbreak();
            return std::nullopt;
        }

        Point3 intersectionPoint = ray.o + hitInfo.tIntersection * ray.d;
        hitInfo.normal = (intersectionPoint - center) / radius;
        hitInfo.debug_position = intersectionPoint;

        return hitInfo;
    }

        std::optional<HitInfo> checkIntersectionAndClosestHit_Old(const Ray& ray) const {
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
        //std::cout << hitInfo.tIntersection << "aaaaa" << std::endl;
        if(hitInfo.tIntersection < ray.tmin || hitInfo.tIntersection > ray.tmax) {
          //  __debugbreak();
            return std::nullopt;
        }

        Point3 intersectionPoint = ray.o + hitInfo.tIntersection * ray.d;
        hitInfo.normal = (intersectionPoint - center) / radius;
        hitInfo.debug_position = intersectionPoint;

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