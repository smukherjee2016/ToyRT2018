#pragma once

#include "common/common.hpp"

enum typeOfVertex {
    SURFACE,
    EMITTER,
    SENSOR
};

struct HitBundle {
    HitInfo hitInfo;
    std::shared_ptr<Object> closestObject;

};

struct Vertex {
    HitBundle hitPointAndMaterial;
    Spectrum bsdf_xi_xiplus1;
    Float pdfBSDFW;
    Float pdfBSDFA;
    Float G_xi_xiplus1;
    typeOfVertex vertexType = SURFACE;
};

class Path {
public:
    std::vector<Vertex> vertices;
};