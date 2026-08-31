cmake_minimum_required(VERSION 3.25)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    message(FATAL_ERROR
        "macOS is unsupported: gpu-particle-singularity requires OpenGL 4.6 Core and GLSL 460."
    )
endif()

project(
    gpu_particle_singularity
    VERSION 0.1.0
    DESCRIPTION "GPU particle singularity simulation"
    LANGUAGES C CXX
)

if(APPLE)
    message(FATAL_ERROR
        "macOS is unsupported: gpu-particle-singularity requires OpenGL 4.6 Core and GLSL 460."
    )
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(ProjectOptions)
include(CTest)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_subdirectory(third_party/glad)

add_executable(gpu_particle_singularity)
target_sources(
    gpu_particle_singularity
    PRIVATE
        src/main.cpp
        src/bootstrap/glfw_context.cpp
        src/bootstrap/glfw_context.hpp
        src/demo/gps_demo.cpp
        src/demo/gps_demo.hpp
        src/graphics/particle_buffer.cpp
        src/graphics/particle_buffer.hpp
        src/graphics/particle_gpu.hpp
        src/graphics/shader_program.cpp
        src/graphics/shader_program.hpp
        src/simulation/particle_initializer.cpp
        src/simulation/particle_initializer.hpp
        src/simulation/xorshift32.cpp
        src/simulation/xorshift32.hpp
        src/support/app_options.hpp
        src/support/opengl_diagnostics.cpp
        src/support/opengl_diagnostics.hpp
        src/support/smoke_test.cpp
        src/support/smoke_test.hpp
        src/ui/imgui_session.cpp
        src/ui/imgui_session.hpp
)
set_target_properties(
    gpu_particle_singularity
    PROPERTIES
        CXX_EXTENSIONS OFF
        OUTPUT_NAME gpu-particle-singularity
)
target_include_directories(gpu_particle_singularity PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_definitions(gpu_particle_singularity PRIVATE GLFW_INCLUDE_NONE)
target_link_libraries(
    gpu_particle_singularity
    PRIVATE
        glad2
        glfw
        glm::glm
        imgui::imgui
)
gps_configure_target(gpu_particle_singularity)

if(BUILD_TESTING)
    find_package(Catch2 3 CONFIG REQUIRED)

    add_executable(
        gpu_particle_singularity_tests
        tests/app_options_test.cpp
        tests/particle_initializer_test.cpp
        src/simulation/particle_initializer.cpp
        src/simulation/particle_initializer.hpp
        src/simulation/xorshift32.cpp
        src/simulation/xorshift32.hpp
    )
    set_target_properties(gpu_particle_singularity_tests PROPERTIES CXX_EXTENSIONS OFF)
    target_include_directories(
        gpu_particle_singularity_tests
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )
    target_link_libraries(
        gpu_particle_singularity_tests
        PRIVATE
            Catch2::Catch2WithMain
            glm::glm
    )
    gps_configure_target(gpu_particle_singularity_tests)

    include(Catch)
    catch_discover_tests(gpu_particle_singularity_tests)
endif()
