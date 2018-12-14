#pragma once

#include "camera/camera.hpp"

class PinholeCamera : public Camera {

public:
    Point3 origin;
    Point3 lookAt; //lookAt is a point, not direction
    Vector3 up;
    Vector3 directionToLookAt;

    Vector3 C_x, C_y, C_z;
    void makeBasisVectors() {
        directionToLookAt = glm::normalize(lookAt - origin);

        //Basis vectors at camera origin
        C_x = glm::normalize(glm::cross(directionToLookAt, up));
        C_y = glm::normalize(glm::cross(C_x, directionToLookAt));
        C_z = glm::normalize(-directionToLookAt);
    }

    PinholeCamera(Point3 _origin, Point3 _lookAt, Vector3 _up) {
        origin = _origin;
        lookAt = _lookAt;
        up = _up;

        makeBasisVectors();
    }

    Ray generateCameraRay(const int x, const int y, std::shared_ptr<Film> film) const override { //Pixel coordinates
        Float u = (static_cast<Float>(x) + rng.generate1DUniform()) / film->screenWidth; //Shoot ray randomly to do AntiAliasing
        Float v = (static_cast<Float>(y) + rng.generate1DUniform()) / film->screenHeight;
#ifdef USE_X_FOV
        Float widthImagePlane = 2.0 * film->distanceToFilm * std::tan(film->FOV / 2.0);
        Float heightImagePlane = widthImagePlane / film->aspectRatio;
#else
        Float heightImagePlane = 2.0 * film->distanceToFilm * std::tan(film->FOV / 2.0);
        Float widthImagePlane = heightImagePlane * film->aspectRatio;
#endif
        Float xImagePlane = (u - 0.5) * widthImagePlane;
        Float yImagePlane = (v - 0.5) * heightImagePlane;

        Point3 positionOfPixelInWorld = origin + film->distanceToFilm * directionToLookAt
                + xImagePlane * C_x + yImagePlane * C_y;

        Vector3 directionInImageWorld = glm::normalize(positionOfPixelInWorld - origin);

        Ray ray(origin, directionInImageWorld);
        return ray;

    }
};