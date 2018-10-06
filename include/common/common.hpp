#pragma once

#include <vector>
#define GLM_FORCE_INLINE
#include <glm/glm.hpp>
#define _USE_MATH_DEFINES
#include <cmath>
#include <fstream>
//#define USE_X_FOV
//#define USE_DOUBLE_PRECISION

#ifdef USE_DOUBLE_PRECISION
using Point2 = glm::dvec2;
using Point3 = glm::dvec3;
using Vector3 = glm::dvec3;
using Float = double;
using Spectrum = glm::dvec3;
#else
using Point2 = glm::vec2;
using Point3 = glm::vec3;
using Vector3 = glm::vec3;
using Float = float;
using Spectrum = glm::vec3;
#endif

const Float Infinity = std::numeric_limits<Float>::max();
const Float M_PI = 3.141592653589793238462643383279502884197169399375105;
const Float M_INVPI = 1.0 / M_PI;


class Ray
{
public:
    Point3 o;
    Vector3 d;
    Float t;
    Float tmin, tmax;

Ray(const Point3& _o = Vector3(0), const Vector3& _d = Vector3(0), const Float& _t = Infinity, const Float& _tmin = 1e-5,
    const Float& _tmax = Infinity):
    o(_o), d(_d), t(_t), tmin(_tmin), tmax(_tmax) { }

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


    //TODO- Replace by Pixar paper later sometime
    void makeOrthonormalBasis(const Vector3& normal) {
        Vector3 in(0.0, 1.0, 0.0); //Arbitrary, set by ourselves
        if(std::abs(glm::dot(in, normal)) > 0.99) { //If the vectors are too close
            in = Vector3(0.0, 0.0, 1.0); //Select another vector
        }

        //Theta = [0, 2*PI], phi = [0, PI/2] for hemisphere, [0,PI] for sphere
        Cz = glm::normalize(glm::cross(normal, in));
        Cx = glm::normalize(glm::cross(normal, Cz));
        Cy = glm::normalize(normal);


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

    Float theta = 2 * M_PI * u1;
    Float phi = std::acos(u2);

    thetaphi.x = theta;
    thetaphi.y = phi;

    return thetaphi;
}

inline Point2 uniformSphereSample(const Float u1, const Float u2) {
    Point2 thetaphi;

    Float theta = 2 * M_PI * u1;
    Float phi = std::acos(1.0 - 2.0 * u2);

    thetaphi.x = theta;
    thetaphi.y = phi;

    return thetaphi;
}