#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cassert>
#include <cstdint>
#include <string_view>
#include <unordered_map>

static constexpr const char* Title = "Minicraft Plus Plus";
static constexpr int Width = 256;
static constexpr int Height = 144;

static constexpr const char* Spritesheet = "spritesheet.png";
static constexpr const char* Font = "RasterForgeRegular.ttf";

/* TODO: fonts have to be rendered at a very high resolution to avoid aliasing */
static constexpr int FontResolution = 4;

static constexpr auto Presentation = SDL_LOGICAL_PRESENTATION_LETTERBOX;
static constexpr int VSync = 1;
static constexpr auto Flash = SDL_FLASH_BRIEFLY;
static constexpr auto PixelFormat = SDL_PIXELFORMAT_INDEX8;
static constexpr auto BlendMode = SDL_BLENDMODE_BLEND;
static constexpr const char* LowPower = "1";
static constexpr const char* Driver = nullptr;

/* TODO: why does PIXELART look like linear filtering */
static constexpr auto ScaleMode = SDL_SCALEMODE_NEAREST;

static SDL_Window* window;
static SDL_Renderer* renderer;
static TTF_TextEngine* textEngine;
static SDL_Surface* spritesheet;
static SDL_Palette* palette;

/* TODO: waiting for MSVC to support std::flat_map */
static std::unordered_map<uint64_t, SDL_Surface*> spriteSurfaces;
static std::unordered_map<uint64_t, SDL_Texture*> spriteTextures;
static std::unordered_map<int, TTF_Font*> fonts;
static std::unordered_map<uint64_t, SDL_Texture*> fontTextures;

static float cameraX;
static float cameraY;

/**
 * colors have 6 values per channel
 * e.g. for 512, red is 5, green is 1, blue is 2
 */
static SDL_Color getColor(uint64_t inColor)
{
    uint8_t red = (inColor / 100) % 10;
    uint8_t green = (inColor / 10) % 10;
    uint8_t blue = inColor % 10;

    assert(red >= 0 && red <= 5);
    assert(green >= 0 && green <= 5);
    assert(blue >= 0 && blue <= 5);

    SDL_Color color;
    color.r = red * 255 / 5;
    color.g = green * 255 / 5;
    color.b = blue * 255 / 5;
    color.a = 255;
    return color;
}

static uint64_t getSpriteSurfaceHash(uint64_t sprite)
{
    return (sprite >> 40) & 0x3FFFFF;
}

static uint64_t getSpriteTextureHash(uint64_t sprite)
{
    return sprite;
}

static SDL_Color getSpriteColor1(uint64_t sprite)
{
    return getColor((sprite >> 0) & 0x3FF);
}

static SDL_Color getSpriteColor2(uint64_t sprite)
{
    return getColor((sprite >> 10) & 0x3FF);
}

static SDL_Color getSpriteColor3(uint64_t sprite)
{
    return getColor((sprite >> 20) & 0x3FF);
}

static SDL_Color getSpriteColor4(uint64_t sprite)
{
    return getColor((sprite >> 30) & 0x3FF);
}

static uint64_t getSpriteX(uint64_t sprite)
{
    return (sprite >> 40) & 0x1FF;
}

static uint64_t getSpriteY(uint64_t sprite)
{
    return (sprite >> 49) & 0x1FF;
}

static uint64_t getSpriteSize(uint64_t sprite)
{
    return ((sprite >> 58) & 0xF) + 1;
}

static bool initSDL()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return false;
    }

    SDL_SetHint(SDL_HINT_RENDER_DRIVER, Driver);
    SDL_SetHint(SDL_HINT_RENDER_GPU_LOW_POWER, LowPower);

    window = SDL_CreateWindow(Title, 960, 720, SDL_WINDOW_HIDDEN);
    if (!window)
    {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return false;
    }

    renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer)
    {
        SDL_Log("Failed to create renderer: %s", SDL_GetError());
        return false;
    }

    SDL_SetRenderLogicalPresentation(renderer, Width, Height, Presentation);
    SDL_SetRenderVSync(renderer, VSync);
    SDL_SetDefaultTextureScaleMode(renderer, ScaleMode);

    SDL_ShowWindow(window);
    SDL_SetWindowResizable(window, true);
    SDL_FlashWindow(window, Flash);

    return true;
}

static bool initTTF()
{
    if (!TTF_Init())
    {
        SDL_Log("Failed to initialize SDL ttf: %s", SDL_GetError());
        return false;
    }

    textEngine = TTF_CreateRendererTextEngine(renderer);
    if (!textEngine)
    {
        SDL_Log("Failed to create text engine: %s", SDL_GetError());
        return false;
    }

    return true;
}

static bool initSpritesheet()
{
    spritesheet = IMG_Load(Spritesheet);
    if (!spritesheet)
    {
        SDL_Log("Failed to load spritesheet: %s", SDL_GetError());
        return false;
    }

    palette = SDL_CreatePalette(5);
    if (!palette)
    {
        SDL_Log("Failed to create palette: %s", SDL_GetError());
        return false;
    }

    return true;
}

bool mppRendererInit()
{
    if (!initSDL())
    {
        SDL_Log("Failed to initialize SDL");
        return false;
    }

    if (!initTTF())
    {
        SDL_Log("Failed to initialize TTF");
        return false;
    }

    if (!initSpritesheet())
    {
        SDL_Log("Failed to initialize spritesheet");
        return false;
    }

    return true;
}

static void freeMaps()
{
    for (auto& [sprite, surface] : spriteSurfaces)
    {
        SDL_DestroySurface(surface);
    }

    for (auto& [sprite, texture] : spriteTextures)
    {
        SDL_DestroyTexture(texture);
    }

    for (auto& [size, font] : fonts)
    {
        TTF_CloseFont(font);
    }

    for (auto& [hash, texture] : fontTextures)
    {
        SDL_DestroyTexture(texture);
    }

    spriteSurfaces.clear();
    spriteTextures.clear();
    fonts.clear();
    fontTextures.clear();
}

void mppRendererQuit()
{
    SDL_HideWindow(window);

    freeMaps();

    SDL_DestroySurface(spritesheet);
    SDL_DestroyPalette(palette);

    TTF_DestroyRendererTextEngine(textEngine);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    TTF_Quit();
    SDL_Quit();
}

void mppRendererClear()
{
    SDL_RenderClear(renderer);
}

void mppRendererPresent()
{
    SDL_RenderPresent(renderer);
}

void mppRendererSetCamera(float x, float y)
{
    cameraX = x - Width / 2;
    cameraY = y - Height / 2;
}

void mppRendererGetCameraBounds(float& x1, float& y1, float& x2, float& y2)
{
    x1 = cameraX - Width / 2;
    y1 = cameraY - Height / 2;

    x2 = cameraX + Width / 2;
    y2 = cameraY + Height / 2;
}

static SDL_Surface* createSpriteSurface(uint64_t sprite)
{
    uint64_t size = getSpriteSize(sprite);

    SDL_Rect rect;
    rect.x = getSpriteX(sprite);
    rect.y = getSpriteY(sprite);
    rect.w = size;
    rect.h = size;

    SDL_Surface* surface = SDL_CreateSurface(size, size, PixelFormat);
    if (!surface)
    {
        SDL_Log("Failed to create surface: %s", SDL_GetError());
        return nullptr;
    }

    SDL_BlitSurface(spritesheet, &rect, surface, nullptr);

    return surface;
}

static SDL_Texture* createSpriteTexture(uint64_t sprite, SDL_Surface* surface)
{
    SDL_Color colors[5];
    colors[0] = getSpriteColor1(sprite);
    colors[1] = getSpriteColor2(sprite);
    colors[2] = getSpriteColor3(sprite);
    colors[3] = getSpriteColor4(sprite);
    colors[4].a = 0.0f;

    SDL_SetPaletteColors(palette, colors, 0, 5);
    SDL_SetSurfacePalette(surface, palette);

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture)
    {
        SDL_Log("Failed to create texture: %s", SDL_GetError());
        return nullptr;
    }

    SDL_SetTextureBlendMode(texture, BlendMode);

    return texture;
}

void mppRendererDraw(uint64_t sprite, float x, float y)
{
    uint64_t surfaceHash = getSpriteSurfaceHash(sprite);
    uint64_t textureHash = getSpriteTextureHash(sprite);

    auto surface = spriteSurfaces.find(surfaceHash);
    if (surface == spriteSurfaces.end())
    {
        SDL_Surface* surfacePtr = createSpriteSurface(sprite);
        surface = spriteSurfaces.emplace(surfaceHash, surfacePtr).first;
    }
    if (!surface->second)
    {
        return;
    }

    auto texture = spriteTextures.find(textureHash);
    if (texture == spriteTextures.end())
    {
        SDL_Texture* texturePtr = createSpriteTexture(sprite, surface->second);
        texture = spriteTextures.emplace(textureHash, texturePtr).first;
    }
    if (!texture->second)
    {
        return;
    }

    uint64_t size = getSpriteSize(sprite);

    SDL_FRect rect;
    rect.x = x - size / 2 - cameraX;
    rect.y = y - size / 2 - cameraY;
    rect.w = size;
    rect.h = size;

    SDL_RenderTexture(renderer, texture->second, nullptr, &rect);
}

static uint64_t getFontTextureHash(const char* text, int color, int size)
{
    /* TODO: collisions */

    uint64_t hash = 0;

    hash |= std::hash<std::string_view>{}(text);
    hash ^= std::hash<int>{}(color);
    hash ^= std::hash<int>{}(size);

    return hash;
}

static SDL_Texture* createFontTexture(const char* text, float x, float y, int inColor, TTF_Font* font)
{
    SDL_Color color = getColor(inColor);

    SDL_Surface* surface = TTF_RenderText_Blended(font, text, 0, color);
    if (!surface)
    {
        SDL_Log("Failed to create surface: %s", SDL_GetError());
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    if (!texture)
    {
        SDL_Log("Failed to create texture: %s", SDL_GetError());
        return nullptr;
    }

    return texture;
}

void mppRendererDraw(const char* text, float x, float y, int color, int size)
{
    size *= FontResolution;

    auto font = fonts.find(size);
    if (font == fonts.end())
    {
        TTF_Font* fontPtr = TTF_OpenFont(Font, size);
        if (!fontPtr)
        {
            SDL_Log("Failed to open font: %s", SDL_GetError());
        }
        font = fonts.emplace(size, fontPtr).first;
    }
    if (!font->second)
    {
        return;
    }

    uint64_t textureHash = getFontTextureHash(text, color, size);

    auto texture = fontTextures.find(textureHash);
    if (texture == fontTextures.end())
    {
        SDL_Texture* texturePtr = createFontTexture(text, x, y, color, font->second);
        texture = fontTextures.emplace(textureHash, texturePtr).first;
    }
    if (!texture->second)
    {
        return;
    }

    SDL_FRect rect;
    rect.w = (texture->second->w) / FontResolution;
    rect.h = (texture->second->h) / FontResolution;
    rect.x = x - rect.w / 2 - cameraX;
    rect.y = y - rect.h / 2 - cameraY;

    SDL_RenderTexture(renderer, texture->second, nullptr, &rect);
}