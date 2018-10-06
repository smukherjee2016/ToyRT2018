#pragma once

#include <random>
#include "pcg_random.hpp"
#include "common/common.hpp"

class RNG {
public:
    RNG() {
        pcg_extras::seed_seq_from<std::random_device> seed_source;
        rng.seed(seed_source);
    }

    Float generate1DUniform() {
        std::uniform_real_distribution<Float> uniformRealDistribution(0.0, 1.0);
        return uniformRealDistribution(rng);
    }

    int generateRandomInt(int maxValue) {
        std::uniform_int_distribution<int> uniformIntDistribution(0, maxValue);
        return uniformIntDistribution(rng);
    }

private:
    pcg64_oneseq rng;

};

static thread_local RNG rng;