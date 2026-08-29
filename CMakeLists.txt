cmake_minimum_required(VERSION 3.25)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    message(FATAL_ERROR
        "macOS is unsupported: opengl-lesson-bootstrap requires OpenGL 4.6 Core and GLSL 460."
    )
endif()

project(
    opengl_lesson_bootstrap
    VERSION 0.1.0
    DESCRIPTION "Domain-neutral OpenGL lesson starter"
    LANGUAGES C CXX
)

if(APPLE)
    message(FATAL_ERROR
        "macOS is unsupported: opengl-lesson-bootstrap requires OpenGL 4.6 Core and GLSL 460."
    )
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(ProjectOptions)
include(CTest)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_subdirectory(third_party/glad)

add_executable(opengl_lesson_bootstrap)
target_sources(
    opengl_lesson_bootstrap
    PRIVATE
        src/main.cpp
        src/bootstrap/glfw_context.cpp
        src/bootstrap/glfw_context.hpp
        src/demo/triangle_demo.cpp
        src/demo/triangle_demo.hpp
        src/graphics/shader_program.cpp
        src/graphics/shader_program.hpp
        src/support/app_options.hpp
        src/support/opengl_diagnostics.cpp
        src/support/opengl_diagnostics.hpp
        src/support/smoke_test.cpp
        src/support/smoke_test.hpp
        src/ui/imgui_session.cpp
        src/ui/imgui_session.hpp
)
set_target_properties(
    opengl_lesson_bootstrap
    PROPERTIES
        CXX_EXTENSIONS OFF
        OUTPUT_NAME opengl-lesson-bootstrap
)
target_include_directories(opengl_lesson_bootstrap PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_definitions(opengl_lesson_bootstrap PRIVATE GLFW_INCLUDE_NONE)
target_link_libraries(
    opengl_lesson_bootstrap
    PRIVATE
        glad2
        glfw
        glm::glm
        imgui::imgui
)
olb_configure_target(opengl_lesson_bootstrap)

if(BUILD_TESTING)
    find_package(Catch2 3 CONFIG REQUIRED)

    add_executable(opengl_lesson_bootstrap_tests tests/app_options_test.cpp)
    set_target_properties(opengl_lesson_bootstrap_tests PROPERTIES CXX_EXTENSIONS OFF)
    target_include_directories(
        opengl_lesson_bootstrap_tests
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )
    target_link_libraries(opengl_lesson_bootstrap_tests PRIVATE Catch2::Catch2WithMain)
    olb_configure_target(opengl_lesson_bootstrap_tests)

    include(Catch)
    catch_discover_tests(opengl_lesson_bootstrap_tests)
endif()
