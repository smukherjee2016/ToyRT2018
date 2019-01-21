#pragma once

#include "common/common.hpp"
#include "film/film.hpp"
#include "util/sampler.hpp"

class Camera {
public:
   virtual Ray generateCameraRay(const int x, const int y, std::shared_ptr<Film> film) const = 0;

protected:
    virtual ~Camera(){}
};