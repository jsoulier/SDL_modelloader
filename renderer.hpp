#pragma once

#include <cassert>
#include <cstdint>

static constexpr uint64_t Black = 0;
static constexpr uint64_t Gray = 222;
static constexpr uint64_t Red = 500;
static constexpr uint64_t Green = 50;
static constexpr uint64_t Blue = 5;
static constexpr uint64_t Yellow = 550;
static constexpr uint64_t Cyan = 55;
static constexpr uint64_t Magenta = 505;
static constexpr uint64_t White = 555;

/**
 * 00-09: color1
 * 10-19: color2
 * 20-29: color3
 * 30-39: color4
 * 40-48: x
 * 49-57: y
 * 58-61: size
 * 62-63: unused
 */
constexpr uint64_t mppRendererCreateSprite(
    uint64_t color1,
    uint64_t color2,
    uint64_t color3,
    uint64_t color4,
    uint64_t x,
    uint64_t y,
    uint64_t size)
{
    assert(color1 < 1024);
    assert(color2 < 1024);
    assert(color3 < 1024);
    assert(color4 < 1024);
    assert(x < 256);
    assert(y < 256);
    assert(size > 0 && size <= 16);

    uint64_t sprite = 0;
    sprite |= color1 << 0;
    sprite |= color2 << 10;
    sprite |= color3 << 20;
    sprite |= color4 << 30;
    sprite |= x << 40;
    sprite |= y << 49;
    sprite |= (size - 1) << 58;
    return sprite;
}

bool mppRendererInit();
void mppRendererQuit();
void mppRendererClear();
void mppRendererPresent();
void mppRendererSetCamera(float x, float y);
void mppRendererGetCameraBounds(float& x1, float& y1, float& x2, float& y2);
void mppRendererDraw(uint64_t sprite, float x, float y);
void mppRendererDraw(const char* text, float x, float y, int color, int size);