#include "bootstrap/glfw_context.hpp"
#include "demo/gps_demo.hpp"
#include "support/app_options.hpp"
#include "support/opengl_diagnostics.hpp"
#include "support/smoke_test.hpp"
#include "ui/imgui_session.hpp"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <exception>
#include <glad/gl.h>
#include <iostream>
#include <stdexcept>

namespace {

constexpr int window_width = 1280;
constexpr int window_height = 720;
constexpr double maximum_temporary_frame_delta_seconds = 1.0 / 30.0;

int run(const gps::AppOptions& options) {
    const gps::GlfwSession glfw_session{};
    gps::Window window =
        gps::create_window(window_width, window_height, "gpu-particle-singularity");
    glfwMakeContextCurrent(window.get());

    const int loaded_version = gps::load_opengl();
    gps::initialize_opengl_diagnostics(loaded_version);

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    gps::SmokeTest smoke_test{options.run_mode};
    glfwSwapInterval(smoke_test.enabled() ? 0 : 1);

    // These objects must be destroyed while their OpenGL context is still current.
    const gps::ImGuiSession imgui{window.get()};
    gps::GpsDemo particle_demo{};
    double previous_time = glfwGetTime();

    while (glfwWindowShouldClose(window.get()) == GLFW_FALSE) {
        glfwPollEvents();
        imgui.begin_frame();

        const gps::ParticleControlEvents control_events = particle_demo.draw_controls();
        const double now = glfwGetTime();
        const double frame_delta =
            control_events.particles_reset
                ? 0.0
                : std::clamp(now - previous_time, 0.0, maximum_temporary_frame_delta_seconds);
        previous_time = now;

        int framebuffer_width = 0;
        int framebuffer_height = 0;
        glfwGetFramebufferSize(window.get(), &framebuffer_width, &framebuffer_height);
        if (framebuffer_width > 0 && framebuffer_height > 0) {
            glViewport(0, 0, framebuffer_width, framebuffer_height);
            glClearColor(0.025F, 0.035F, 0.055F, 1.0F);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            particle_demo.draw(framebuffer_width, framebuffer_height,
                               static_cast<float>(frame_delta));
        }
        imgui.render();

        smoke_test.frame_rendered();
        glfwSwapBuffers(window.get());

        if (smoke_test.complete()) {
            glfwSetWindowShouldClose(window.get(), GLFW_TRUE);
        }
    }

    smoke_test.verify_complete();
    return 0;
}

} // namespace

// Project code throws std::exception types, and standard streams keep their non-throwing default.
// NOLINTNEXTLINE(bugprone-exception-escape)
int main(int argc, char** argv) {
    try {
        gps::AppOptions options{};
        try {
            options = gps::parse_app_options(argc, argv);
        } catch (const std::invalid_argument& exception) {
            std::cerr << exception.what() << '\n' << gps::usage();
            return 2;
        }

        if (options.show_help) {
            std::cout << gps::usage();
            return 0;
        }

        return run(options);
    } catch (const std::exception& exception) {
        std::cerr << "Fatal error: " << exception.what() << '\n';
        return 1;
    }
}
