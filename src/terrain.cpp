
#include "terrain.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <random>
#include <chrono>
#include <limits>

class PerlinNoise {
private:
    std::array<int, 512> permutation{};

    static double fade(double t) {
        return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
    }

    static double lerp(double t, double a, double b) {
        return a + t * (b - a);
    }

    static double grad(
        int hash,
        double x,
        double y,
        double z
    ) {
        int h = hash & 15;

        double u = h < 8 ? x : y;

        double v = h < 4 ? y :
                   (h == 12 || h == 14 ? x : z);

        return ((h & 1) ? -u : u) +
               ((h & 2) ? -v : v);
    }

public:
    explicit PerlinNoise(std::uint32_t seed) {
        std::array<int, 256> p;

        std::iota(p.begin(), p.end(), 0);

        std::mt19937 generator(seed);

        std::shuffle(
            p.begin(),
            p.end(),
            generator
        );

        for (int i = 0; i < 512; i++) {
            permutation[i] = p[i & 255];
        }
    }

    double noise(double x, double y, double z) const {

        int X = static_cast<int>(std::floor(x)) & 255;
        int Y = static_cast<int>(std::floor(y)) & 255;
        int Z = static_cast<int>(std::floor(z)) & 255;

        x -= std::floor(x);
        y -= std::floor(y);
        z -= std::floor(z);

        double u = fade(x);
        double v = fade(y);
        double w = fade(z);

        int A  = permutation[X] + Y;
        int AA = permutation[A] + Z;
        int AB = permutation[A + 1] + Z;

        int B  = permutation[X + 1] + Y;
        int BA = permutation[B] + Z;
        int BB = permutation[B + 1] + Z;

        double result = lerp(
            w,

            lerp(
                v,

                lerp(
                    u,
                    grad(permutation[AA], x, y, z),
                    grad(permutation[BA], x - 1, y, z)
                ),

                lerp(
                    u,
                    grad(permutation[AB], x, y - 1, z),
                    grad(permutation[BB], x - 1, y - 1, z)
                )
            ),

            lerp(
                v,

                lerp(
                    u,
                    grad(permutation[AA + 1], x, y, z - 1),
                    grad(permutation[BA + 1], x - 1, y, z - 1)
                ),

                lerp(
                    u,
                    grad(permutation[AB + 1], x, y - 1, z - 1),
                    grad(permutation[BB + 1], x - 1, y - 1, z - 1)
                )
            )
        );

        // Normalize approximately into [-1, 1].
        return std::clamp(result * 0.707, -1.0, 1.0);
    }
};


static std::uint32_t createWorldSeed() {

    std::random_device randomDevice;

    std::uint64_t clockValue =
        static_cast<std::uint64_t>(
            std::chrono::high_resolution_clock::now()
                .time_since_epoch().count()
        );

    return static_cast<std::uint32_t>(
        clockValue ^
        (clockValue >> 32) ^
        (static_cast<std::uint64_t>(randomDevice()) << 16) ^
        randomDevice()
    );
}


std::vector<TerrainBlock> generateTerrain(
    int size,
    int depth,
    std::uint32_t* seedOut
) {
    std::vector<TerrainBlock> world;

    if (size <= 0 || depth <= 0)
        return world;

    std::uint32_t seed = createWorldSeed();

    if (seedOut)
        *seedOut = seed;

    PerlinNoise noise(seed);

    // Reserve approximate memory for every terrain block.
    world.reserve(
        static_cast<std::size_t>(size) *
        size *
        depth
    );

    std::mt19937 oreRandom(seed ^ 0xA53C9E17u);

    std::cout << "Generating world...\n";

    for (int x = 0; x < size; x++) {
        for (int z = 0; z < size; z++) {


            double continental = noise.noise(
                x * 0.012,
                15.0,
                z * 0.012
            );

            double hills = noise.noise(
                x * 0.035,
                42.0,
                z * 0.035
            );

            double detail = noise.noise(
                x * 0.09,
                83.0,
                z * 0.09
            );

            // Combine different noise frequencies.
            double combined =
                continental * 0.55 +
                hills * 0.30 +
                detail * 0.15;

            int surfaceY = 20 +
                static_cast<int>(
                    std::round(combined * 30.0)
                );


            double moisture = noise.noise(
                x * 0.025 + 100.0,
                125.0,
                z * 0.025 + 100.0
            );

            bool sandySurface = moisture < -0.40;

            int bottomY = surfaceY - depth + 1;


            for (int y = bottomY; y <= surfaceY; y++) {

                int depthFromSurface = surfaceY - y;

                int blockId = 4; // Default: stone

                if (depthFromSurface == 0) {

                    blockId = sandySurface ? 3 : 1;
                }
                else if (depthFromSurface <= 3) {

                    blockId = sandySurface ? 3 : 2;
                }
                else {

                    double oreNoise = noise.noise(
                        x * 0.18 + 57.3,
                        y * 0.18 + 23.7,
                        z * 0.18 - 91.2
                    );

                    if (
                        depthFromSurface >= 27 &&
                        oreNoise > 0.66
                    ) {
                        blockId = 8; // Diamond
                    }
                    else if (
                        depthFromSurface >= 21 &&
                        oreNoise > 0.57
                    ) {
                        blockId = 7; // Gold
                    }
                    else if (
                        depthFromSurface >= 12 &&
                        oreNoise > 0.45
                    ) {
                        blockId = 6; // Iron
                    }
                    else if (oreNoise > 0.28) {

                        blockId = 5; // Coal
                    }
                }

                world.push_back({
                    x,
                    y,
                    z,
                    blockId
                });
            }
        }

        if ((x + 1) % 16 == 0) {
            std::cout << "Generated "
                      << (x + 1) << " / "
                      << size << " columns\n";
        }
    }

    std::cout << "World generation complete!\n";
    std::cout << "Seed: " << seed << "\n";
    std::cout << "Total blocks: " << world.size() << "\n";

    return world;
}


int getSurfaceHeight(
    const std::vector<TerrainBlock>& world,
    int x,
    int z
) {
    int highest = std::numeric_limits<int>::min();

    for (const TerrainBlock& block : world) {

        if (block.x == x && block.z == z) {
            highest = std::max(highest, block.y);
        }
    }

    return highest;
}
