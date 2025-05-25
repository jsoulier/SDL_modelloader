#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

struct Voxel
{
    Voxel(int vx, int vy, int vz, float tx, int nx, int ny, int nz);
    bool operator==(const Voxel other) const;

    /*
     * 00-00: x sign bit
     * 01-07: x (7 bits)
     * 08-08: y sign bit
     * 09-15: y (7 bits)
     * 16-16: z sign bit
     * 17-23: z (7 bits)
     * 24-26: direction (3 bits)
     * 27-31: unused (5 bits)
     * 32-63: texcoord (32 bits)
     */
    uint32_t packed;
    float texcoord;
};

namespace std
{
    template<>
    struct hash<Voxel>
    {
        size_t operator()(const Voxel voxel) const
        {
            size_t h1 = std::hash<uint32_t>{}(voxel.packed);
            size_t h2 = std::hash<float>{}(voxel.texcoord);
            return h1 ^ h2;
        }
    };
}