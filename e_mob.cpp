#include <cmath>
#include <cstdint>
#include <limits>

#include "e_mob.hpp"
#include "renderer.hpp"

static constexpr float Speed = 0.05f;

void MppEntityMob::render()
{
    mppRendererDraw("here", x + 0.5f, y + 0.5f, Black, 6);
    mppRendererDraw("here", x, y, Gray, 6);
    mppRendererDraw("here", x - 0.5f, y - 0.5f, White, 6);
    mppRendererSetCamera(x, y);
}

void MppEntityMob::update(uint64_t dt, uint64_t time)
{
    int dx = 0;
    int dy = 0;

    move(dx, dy);

    float length = std::hypotf(dx, dy);
    if (length < std::numeric_limits<float>::epsilon())
    {
        return;
    }

    float speed = getSpeed();
    x += dx * Speed * speed * dt / length;
    y += dy * Speed * speed * dt / length;
}