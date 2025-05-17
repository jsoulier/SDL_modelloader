#include <cassert>
#include <cmath>
#include <cstdint>

#include "voxel.hpp"

Voxel::Voxel(int vx, int vy, int vz, int tx, int nx, int ny, int nz)
{
    packed = 0;
    texcoord = tx;

    assert(vx >= -16 && vx <= 16);
    assert(vy >= -16 && vy <= 16);
    assert(vz >= -16 && vz <= 16);

    packed |= (std::abs(vx) & 0x7F) << 0;
    packed |= (vx < 0) << 7;
    packed |= (std::abs(vy) & 0x7F) << 8;
    packed |= (vy < 0) << 15;
    packed |= (std::abs(vz) & 0x7F) << 16;
    packed |= (vz < 0) << 23;

    uint32_t direction = 0;
    if (nx > 0)
    {
        direction = 0;
    }
    else if (nx < 0)
    {
        direction = 1;
    }
    else if (ny > 0)
    {
        direction = 2;
    }
    else if (ny < 0)
    {
        direction = 3;
    }
    else if (nz > 0)
    {
        direction = 4;
    }
    else if (nz < 0)
    {
        direction = 5;
    }
    else
    {
        assert(false);
    }
    packed |= (direction & 0x7) << 24;
}

bool Voxel::operator==(const Voxel other) const
{
    return packed == other.packed && texcoord == other.texcoord;
}