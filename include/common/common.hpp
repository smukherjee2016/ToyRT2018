#pragma once

#include <vector>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#define _USE_MATH_DEFINES
#include <cmath>

using Point2 = glm::vec2;
using Point3 = glm::vec3;
using Vector3 = glm::vec3;
using Float = float;
using Spectrum = glm::vec3;

const Float Infinity = std::numeric_limits<Float>::max();



class Ray
{
public:
    Point3 o;
    Vector3 d;
    Float t;

Ray(const Point3& _o = Vector3(0), const Vector3& _d = Vector3(0), const Float& _t = Infinity) {
    o = _o;
    d = _d;
    t = _t;
}

};

struct HitInfo {
    Float tIntersection;
    Vector3 normal;
};