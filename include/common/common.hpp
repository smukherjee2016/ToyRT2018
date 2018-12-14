#pragma once

#include <vector>
#define GLM_FORCE_INLINE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#define _USE_MATH_DEFINES
#include <cmath>
#include <fstream>
//#define USE_X_FOV
#define USE_DOUBLE_PRECISION
//#define USE_LIGHT_SAMPLING

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#ifdef USE_DOUBLE_PRECISION
using Point2 = glm::dvec2;
using Point3 = glm::dvec3;
using Vector3 = glm::dvec3;
using Float = double;
using Spectrum = glm::dvec3;

//Ref. https://github.com/nlohmann/json#arbitrary-types-conversions
namespace glm
{
    void to_json(json& j, const dvec3& p)
    {
        j = json{p.r, p.g, p.b};
    }

    void from_json(const json& j, dvec3& p)
    {
        j.at(0).get_to(p.r);
        j.at(1).get_to(p.g);
        j.at(2).get_to(p.b);

    }

    void to_json(json& j, const dvec2& p)
    {
        j = json{p.x, p.y};
    }

    void from_json(const json& j, dvec2& p)
    {
        j.at(0).get_to(p.x);
        j.at(1).get_to(p.y);

    }
}

#else
using Point2 = glm::vec2;
using Point3 = glm::vec3;
using Vector3 = glm::vec3;
using Float = float;
using Spectrum = glm::vec3;

//Ref. https://github.com/nlohmann/json#arbitrary-types-conversions
namespace glm
{
    void to_json(json& j, const vec3& p)
    {
        j = json{p.r, p.g, p.b};
    }

    void from_json(const json& j, vec3& p)
    {
        j.at(0).get_to(p.r);
        j.at(1).get_to(p.g);
        j.at(2).get_to(p.b);

    }

    void to_json(json& j, const vec2& p)
    {
        j = json{p.x, p.y};
    }

    void from_json(const json& j, vec2& p)
    {
        j.at(0).get_to(p.x);
        j.at(1).get_to(p.y);

    }
}
#endif

const Float Infinity = std::numeric_limits<Float>::max();
const Float PI = 3.141592653589793238462643383279502884197169399375105;
const Float M_INVPI = 1.0 / PI;
const Float epsilon = 1e-5;


class Ray
{
public:
    Point3 o;
    Vector3 d;
    Float t;
    Float tmin, tmax;

Ray(const Point3& _o = Vector3(0), const Vector3& _d = Vector3(0), const Float& _tmin = epsilon,
    const Float& _tmax = Infinity):
    o(_o), d(_d), tmin(_tmin), tmax(_tmax) { t = Infinity;}
};

struct HitInfo {
    Float tIntersection;
    Point3 intersectionPoint;
    Vector3 normal;
};

void saveObj(const std::string & filename, const std::vector<Vector3>& points)
{
    std::ofstream fileWriter;
    fileWriter.open(filename, std::ios::binary);

    for (const Vector3 &point : points)
    {
        fileWriter << "v " << point.x << " " << point.y << " " << point.z << std::endl;
    }

    fileWriter.close();
}

class Basis {

public:
    Vector3 Cx;
    Vector3 Cy;
    Vector3 Cz;


    //Ref. Building an Orthonormal Basis, Revisited [Duff et al. 2017]
    void makeOrthonormalBasis(const Vector3& normal) {

        Float sign = std::copysign(1.0, normal.z);
        const Float a = -1.0 / (sign  + normal.z);
        const Float b = normal.x * normal.y * a;

        Cy = glm::normalize(normal);
        Cz = Vector3(1.0 + sign * Cy.x * Cy.x * a, sign * b, -sign * Cy.x);
        Cx = Vector3(b, sign + Cy.y * Cy.y * a, -Cy.y);

    }
};

inline Point3 sphericaltoCartesian(const Float theta, const Float phi) {
    //Y axis up, theta = [0, 2PI], phi=[0,PI/2] for hemisphere
    Point3  point;
    point.x = std::cos(theta) * std::sin(phi);
    point.y = std::cos(phi);
    point.z = std::sin(theta) * std::sin(phi);

    return point;
}

/*
 * All these sampling domain conversions assume:
 * u1, u2 -> [0,1]
 * theta -> [0, 2PI]
 * phi -> [0, PI/2] for hemisphere, [0, PI] for sphere
 */
inline Point2 uniformHemisphereSample(const Float u1, const Float u2) {
    Point2 thetaphi;

    Float theta = 2 * PI * u1;
    Float phi = std::acos(u2);

    thetaphi.x = theta;
    thetaphi.y = phi;

    return thetaphi;
}

inline Point2 uniformSphereSample(const Float u1, const Float u2) {
    Point2 thetaphi;

    Float theta = 2 * PI * u1;
    Float phi = std::acos(1.0 - 2.0 * u2);

    thetaphi.x = theta;
    thetaphi.y = phi;

    return thetaphi;
}