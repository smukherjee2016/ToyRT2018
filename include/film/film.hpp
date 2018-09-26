#pragma once

#include "common/common.hpp"
#include <string>
#include "util/pfmutils.hpp"

class Film {
public:
    Float xFOV = M_PI_4;
    Float distanceToFilm = 1.0;
    Float aspectRatio = 16.0 / 9.0;
    int screenWidth;
    int screenHeight;

    std::vector<Spectrum> pixels;

    Film(Float _xFOV, Float _distanceToFilm, int _screenWidth, int _screenHeight) {
        xFOV = _xFOV;
        distanceToFilm = _distanceToFilm;
        screenWidth = _screenWidth;
        screenHeight = _screenHeight;
        aspectRatio = static_cast<Float>(screenWidth) / screenHeight;
        pixels.resize(screenWidth * screenHeight);
    }

    void writePixels(const std::string file) {
            writePFM(file, pixels, screenWidth, screenHeight);
    }
};