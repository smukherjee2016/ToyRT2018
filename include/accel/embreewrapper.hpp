#pragma once

#include <map>
#include <embree3/rtcore.h>
#include "common/common.hpp"
#include "objects/object.hpp"
#include "hitables/triangle.hpp"

class EmbreeWrapper : public Object
{
    RTCDevice g_device;
    RTCScene g_scene;


public:
    void initAndSetupEmbree() {
        //Create device
        g_device = rtcNewDevice("verbose=3");
        //Create scene
        g_scene = rtcNewScene(g_device);

        //Add meshes

        //Commit changes to scene
        rtcCommitScene(g_scene);

    }

    void cleanUpEmbree() {
        rtcReleaseScene(g_scene);
        g_scene = nullptr;
        rtcReleaseDevice(g_device);
        g_device = nullptr;
    }

    std::optional<HitInfo> checkIntersectionAndClosestHit(const Ray &ray) const override {
        return std::optional<HitInfo>();
    }

    Point3 samplePointOnObject(Sampler sampler) const override {
        return Point3();
    }

    Float pdfSelectPointOnObjectA(const Point3 &point) const override {
        return 0;
    }

    Vector3 getNormalAtPoint(const Point3 &point) const override {
        return Vector3();
    }

    Float surfaceArea() const override {
        return 0;
    }

    bool isEmitter() const override {
        return false;
    }
};