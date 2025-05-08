#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

struct packed_vertex
{
    packed_vertex(int vx, int vy, int vz, int tx, int nx, int ny, int nz);
    bool operator==(const packed_vertex other) const;

    /*
     * 00-00: x sign bit
     * 01-07: x (7 bits)
     * 08-08: y sign bit
     * 09-15: y (7 bits)
     * 16-16: z sign bit
     * 17-23: z (7 bits)
     * 24-26: direction (3 bits)
     * 27-31: unused (5 bits)
     * 32-61: texcoord (32 bits)
     */
    uint32_t packed;
    float texcoord;
};

namespace std
{
    template<>
    struct hash<packed_vertex>
    {
        size_t operator()(const packed_vertex& vertex) const
        {
            size_t h1 = std::hash<uint32_t>{}(vertex.packed);
            size_t h2 = std::hash<float>{}(vertex.texcoord);
            return h1 ^ h2;
        }
    };
}