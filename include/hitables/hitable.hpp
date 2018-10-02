#pragma once

#include <memory>
#include <materials/material.hpp>
#include "common/common.hpp"
#include <optional>


class Hitable {
public:
    virtual std::optional<HitInfo> checkIntersectionAndClosestHit(const Ray& ray) const = 0;
    std::shared_ptr<Material> mat;
};