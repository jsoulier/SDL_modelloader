cmake_minimum_required(VERSION 3.24)
project(lilcraft)

set(BUILD_SHARED_LIBS FALSE)

set(BINARY_DIR ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${BINARY_DIR})
make_directory(${BINARY_DIR})

add_subdirectory(lib/SDL)
add_subdirectory(lib/SDL_ttf)

add_executable(lilcraft WIN32
    lib/fast_obj/fast_obj.c
    lib/sqlite3/sqlite3.c
    lib/stb/stb.c

    src/e/item.c
    src/e/mob.c
    src/e/player.c

    src/data.c
    src/db.c
    src/entity.c
    src/gfx.c
    src/level.c
    src/main.c
    src/math_ex.c
    src/util.c
)

set_target_properties(lilcraft PROPERTIES C_STANDARD 11)

target_include_directories(lilcraft PRIVATE lib/fast_obj)
target_include_directories(lilcraft PRIVATE lib/jsmn)
target_include_directories(lilcraft PRIVATE lib/sqlite3)
target_include_directories(lilcraft PRIVATE lib/stb)

target_link_libraries(lilcraft PRIVATE SDL3::SDL3)
target_link_libraries(lilcraft PRIVATE SDL3_ttf::SDL3_ttf)

if(MSVC)
    target_compile_definitions(lilcraft PRIVATE _USE_MATH_DEFINES)
endif()

if(UNIX)
    target_link_libraries(lilcraft PRIVATE m)
endif()

if(MSVC)
    set(SHADERCROSS lib/SDL_shadercross/msvc/shadercross.exe)
else()
    message("Unsupported platform. Skipping shader building")
endif()

function(add_shader FILE)
    set(DEPENDS ${ARGN})

    set(GLSL ${CMAKE_SOURCE_DIR}/res/shaders/${FILE})
    set(SPV ${CMAKE_SOURCE_DIR}/res/shaders/bin/${FILE}.spv)
    set(DXIL ${CMAKE_SOURCE_DIR}/res/shaders/bin/${FILE}.dxil)
    set(MSL ${CMAKE_SOURCE_DIR}/res/shaders/bin/${FILE}.msl)
    set(JSON ${CMAKE_SOURCE_DIR}/res/shaders/bin/${FILE}.json)

    function(_add_shader PROGRAM SOURCE OUTPUT)
        if(MSVC)
            add_custom_command(
                OUTPUT ${OUTPUT}
                COMMAND ${PROGRAM} ${SOURCE} -o ${OUTPUT}
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                DEPENDS ${SOURCE} ${DEPENDS}
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

add_shader(composite.frag)
add_shader(mesh.frag res/shaders/mesh.glsl)
add_shader(mesh.vert res/shaders/mesh.glsl)
add_shader(readback.comp)
add_shader(screen.vert)

function(add_model NAME)
    configure_file(res/models/${NAME}.mtl ${BINARY_DIR} COPYONLY)
    configure_file(res/models/${NAME}.obj ${BINARY_DIR} COPYONLY)
    configure_file(res/models/${NAME}.png ${BINARY_DIR} COPYONLY)
endfunction()

add_model(dirt_00)
add_model(grass_00)
add_model(player_00)
add_model(sand_00)
add_model(tree_00)
add_model(water_00)

file(COPY LICENSE.txt DESTINATION ${BINARY_DIR})
file(COPY README.md DESTINATION ${BINARY_DIR})