cmake_minimum_required(VERSION 3.24)
project(minicraft++)

set(BINARY_DIR ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BINARY_DIR})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${BINARY_DIR})

make_directory(${BINARY_DIR})

set(SDLIMAGE_AVIF OFF)
set(SDLIMAGE_BMP OFF)
set(SDLIMAGE_GIF OFF)
set(SDLIMAGE_JPG OFF)
set(SDLIMAGE_JXL OFF)
set(SDLIMAGE_LBM OFF)
set(SDLIMAGE_PCX OFF)
set(SDLIMAGE_PNM OFF)
set(SDLIMAGE_QOI OFF)
set(SDLIMAGE_SVG OFF)
set(SDLIMAGE_TGA OFF)
set(SDLIMAGE_TIF OFF)
set(SDLIMAGE_WEBP OFF)
set(SDLIMAGE_XCF OFF)
set(SDLIMAGE_XPM OFF)
set(SDLIMAGE_XV OFF)

add_subdirectory(SDL)
add_subdirectory(SDL_image)
add_subdirectory(SDL_ttf)

add_executable(minicraft++ WIN32
    database.cpp
    e_mob.cpp
    e_player.cpp
    entity.cpp
    item.cpp
    level.cpp
    main.cpp
    renderer.cpp
    sqlite3.c
    tile.cpp
)

set_target_properties(minicraft++ PROPERTIES CXX_STANDARD 23)

target_link_libraries(minicraft++ PRIVATE SDL3::SDL3)
target_link_libraries(minicraft++ PRIVATE SDL3_image::SDL3_image)
target_link_libraries(minicraft++ PRIVATE SDL3_ttf::SDL3_ttf)

configure_file(LICENSE.txt ${BINARY_DIR} COPYONLY)
configure_file(RasterForgeRegular.ttf ${BINARY_DIR} COPYONLY)
configure_file(spritesheet.png ${BINARY_DIR} COPYONLY)