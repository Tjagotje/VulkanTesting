cmake_minimum_required(VERSION 3.20)
project(HelloTriangle LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)

find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(HelloTriangle
    src/Main.cpp
    src/HT.cpp
    src/HT_init.cpp
    src/HT_pipeline.cpp
)


function(add_slang_shader_target TARGET)
    cmake_parse_arguments(SHADER "" "" "SOURCES" ${ARGN})

    set(SHADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
    set(SHADER_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/shaders/slang.spv")
    set(SLANGC_EXECUTABLE "slangc")

    add_custom_command(
        OUTPUT "${SHADER_OUTPUT}"

        COMMAND ${CMAKE_COMMAND} -E make_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/shaders"

        COMMAND "${SLANGC_EXECUTABLE}"
                ${SHADER_SOURCES}
                -target spirv
                -profile spirv_1_4
                -emit-spirv-directly
                -fvk-use-entrypoint-name
                -entry vertMain
                -entry fragMain
                -o "${SHADER_OUTPUT}"

        DEPENDS ${SHADER_SOURCES}
        COMMENT "Compiling Slang shaders"
        VERBATIM
    )

    add_custom_target(${TARGET}
        DEPENDS "${SHADER_OUTPUT}"
    )
endfunction()

set(SHADER_SLANG_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.slang"
)

add_slang_shader_target(Shaders
    SOURCES ${SHADER_SLANG_SOURCES}
)

add_dependencies(HelloTriangle Shaders)


target_include_directories(HelloTriangle PRIVATE src)

target_link_libraries(HelloTriangle PRIVATE
    Vulkan::Vulkan
    glfw
    glm::glm
)

target_precompile_headers(HelloTriangle PRIVATE
    <vulkan/vulkan.hpp>
    <vulkan/vulkan_raii.hpp>
    <glm/glm.hpp>
    <GLFW/glfw3.h>
)


target_compile_definitions(HelloTriangle PRIVATE
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
)