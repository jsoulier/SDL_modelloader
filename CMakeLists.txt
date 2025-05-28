cmake_minimum_required(VERSION 3.24)
project(lilcraft VERSION 0.0.1)

set(BUILD_SHARED_LIBS OFF)

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
set(SDLTTF_PLUTOSVG OFF)

add_subdirectory(lib/FastNoiseLite)
add_subdirectory(lib/SDL)
add_subdirectory(lib/SDL_image)
add_subdirectory(lib/SDL_ttf)
add_subdirectory(lib/glm)
add_subdirectory(lib/json)
add_subdirectory(lib/sqlite3)
add_subdirectory(lib/tinyobjloader)
add_executable(lilcraft WIN32
    src/camera.cpp
    src/database.cpp
    src/entity.cpp
    src/entity/drop.cpp
    src/entity/mob.cpp
    src/entity/player.cpp
    src/item.cpp
    src/loaders.cpp
    src/main.cpp
    src/model.cpp
    src/noise.cpp
    src/renderer.cpp
    src/tile.cpp
    src/voxel.cpp
    src/world.cpp
)
set_target_properties(lilcraft PROPERTIES CXX_STANDARD 23)
target_include_directories(lilcraft PUBLIC include)
target_link_libraries(lilcraft PUBLIC FastNoiseLite)
target_link_libraries(lilcraft PUBLIC SDL3::SDL3)
target_link_libraries(lilcraft PUBLIC SDL3_image::SDL3_image)
target_link_libraries(lilcraft PUBLIC SDL3_ttf::SDL3_ttf)
target_link_libraries(lilcraft PUBLIC glm)
target_link_libraries(lilcraft PUBLIC nlohmann_json)
target_link_libraries(lilcraft PUBLIC sqlite3)
target_link_libraries(lilcraft PUBLIC tinyobjloader)
target_compile_definitions(lilcraft PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)

function(add_shader FILE)
    set(SHADERCROSS lib/SDL_shadercross/shadercross.exe)
    set(GLSL ${CMAKE_SOURCE_DIR}/shaders/${FILE})
    set(SPV ${CMAKE_SOURCE_DIR}/shaders/bin/${FILE}.spv)
    set(DXIL ${CMAKE_SOURCE_DIR}/shaders/bin/${FILE}.dxil)
    set(MSL ${CMAKE_SOURCE_DIR}/shaders/bin/${FILE}.msl)
    set(JSON ${CMAKE_SOURCE_DIR}/shaders/bin/${FILE}.json)
    function(_add_shader TOOL SOURCE OUTPUT)
        if(MSVC)
            add_custom_command(
                OUTPUT ${OUTPUT}
                COMMAND ${TOOL} ${SOURCE} -o ${OUTPUT}
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                DEPENDS ${SOURCE} shaders/voxel.glsl
                BYPRODUCTS ${OUTPUT}
                COMMENT ${OUTPUT}
            )
        endif()
        get_filename_component(NAME ${OUTPUT} NAME)
        set(BINARY ${BINARY_DIR}/${NAME})
        add_custom_command(
            OUTPUT ${BINARY}
            COMMAND ${CMAKE_COMMAND} -E copy ${OUTPUT} ${BINARY}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            DEPENDS ${OUTPUT}
            BYPRODUCTS ${BINARY}
            COMMENT ${BINARY}
        )
        string(REPLACE . _ NAME ${NAME})
        add_custom_target(${NAME} DEPENDS ${BINARY})
        add_dependencies(lilcraft ${NAME})
    endfunction()
    _add_shader(glslc ${GLSL} ${SPV})
    _add_shader(${SHADERCROSS} ${SPV} ${DXIL})
    _add_shader(${SHADERCROSS} ${SPV} ${MSL})
    _add_shader(${SHADERCROSS} ${SPV} ${JSON})
endfunction()
add_shader(model.frag)
add_shader(model.vert)

function(add_model NAME)
    file(COPY models/${NAME}.mtl DESTINATION ${BINARY_DIR})
    file(COPY models/${NAME}.obj DESTINATION ${BINARY_DIR})
    file(COPY models/${NAME}.png DESTINATION ${BINARY_DIR})
endfunction()
add_model(dirt)
add_model(grass)
add_model(invalid)
add_model(player)
add_model(sand)
add_model(tree)
add_model(water)

file(COPY LICENSE.txt DESTINATION ${BINARY_DIR})
file(COPY README.md DESTINATION ${BINARY_DIR})