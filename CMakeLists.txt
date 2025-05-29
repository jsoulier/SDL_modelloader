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
    src/data.c
    src/db.c
    src/e/item.c
    src/e/mob.c
    src/e/player.c
    src/entity.c
    src/main.c
    src/mesh.c
    src/util.c
)
set_target_properties(lilcraft PROPERTIES C_STANDARD 11)
target_include_directories(lilcraft PUBLIC lib/fast_obj)
target_include_directories(lilcraft PUBLIC lib/sqlite3)
target_include_directories(lilcraft PUBLIC lib/stb)
target_link_libraries(lilcraft PUBLIC SDL3::SDL3)
target_link_libraries(lilcraft PUBLIC SDL3_ttf::SDL3_ttf)

function(add_shader FILE)
    if(MSVC)
        set(SHADERCROSS lib/SDL_shadercross/shadercross.exe)
    endif()
    set(GLSL ${CMAKE_SOURCE_DIR}/res/shaders/${FILE})
    set(SPV ${CMAKE_SOURCE_DIR}/res/shaders/bin/${FILE}.spv)
    set(DXIL ${CMAKE_SOURCE_DIR}/res/shaders/bin/${FILE}.dxil)
    set(MSL ${CMAKE_SOURCE_DIR}/res/shaders/bin/${FILE}.msl)
    set(JSON ${CMAKE_SOURCE_DIR}/res/shaders/bin/${FILE}.json)
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

file(COPY LICENSE.txt DESTINATION ${BINARY_DIR})
file(COPY README.md DESTINATION ${BINARY_DIR})