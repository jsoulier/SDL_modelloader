cmake_minimum_required(VERSION 3.24)
project(lilcraft VERSION 0.0.1)

set(BINARY_DIR ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${BINARY_DIR})
make_directory(${BINARY_DIR})

# TODO: Remove when we start using glm symbols
set(BUILD_SHARED_LIBS OFF)

# TODO: Build error
set(SDLIMAGE_AVIF OFF)

add_subdirectory(lib/FastNoiseLite)
add_subdirectory(lib/SDL)
add_subdirectory(lib/SDL_image)
add_subdirectory(lib/SDL_ttf)
add_subdirectory(lib/glm)
add_subdirectory(lib/json)
add_subdirectory(lib/sqlite3)
add_subdirectory(lib/tinyobjloader)
add_executable(lilcraft WIN32
    src/main.cpp
    src/render.cpp
)
set_target_properties(lilcraft PROPERTIES CXX_STANDARD 23)
target_link_libraries(lilcraft PUBLIC
    FastNoiseLite
    SDL3::SDL3
    SDL3_image::SDL3_image
    SDL3_ttf::SDL3_ttf
    glm
    nlohmann_json
    sqlite3
    tinyobjloader
)

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
                DEPENDS ${SOURCE}
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
add_shader(sampler.comp)

file(COPY LICENSE.txt DESTINATION ${BINARY_DIR})
file(COPY README.md DESTINATION ${BINARY_DIR})