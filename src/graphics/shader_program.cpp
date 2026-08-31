#include "graphics/shader_program.hpp"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace gps {
namespace {

class ShaderObject final {
  public:
    explicit ShaderObject(GLuint shader) noexcept : shader_{shader} {}

    ~ShaderObject() {
        if (shader_ != 0) {
            glDeleteShader(shader_);
        }
    }

    ShaderObject(const ShaderObject&) = delete;
    ShaderObject& operator=(const ShaderObject&) = delete;

    ShaderObject(ShaderObject&& other) noexcept : shader_{std::exchange(other.shader_, 0)} {}
    ShaderObject& operator=(ShaderObject&&) = delete;

    [[nodiscard]] GLuint id() const noexcept {
        return shader_;
    }

  private:
    GLuint shader_{0};
};

[[nodiscard]] std::string_view shader_stage_name(ShaderStage stage) noexcept {
    switch (stage) {
    case ShaderStage::vertex:
        return "vertex";
    case ShaderStage::tessellation_control:
        return "tessellation control";
    case ShaderStage::tessellation_evaluation:
        return "tessellation evaluation";
    case ShaderStage::geometry:
        return "geometry";
    case ShaderStage::fragment:
        return "fragment";
    case ShaderStage::compute:
        return "compute";
    default:
        return "unknown-stage";
    }
}

std::string shader_log(GLuint shader) {
    GLint log_length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
    if (log_length <= 1) {
        return {};
    }

    std::string log(static_cast<std::size_t>(log_length), '\0');
    GLsizei written = 0;
    glGetShaderInfoLog(shader, log_length, &written, log.data());
    log.resize(static_cast<std::size_t>(written));
    return log;
}

std::string program_log(GLuint program) {
    GLint log_length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
    if (log_length <= 1) {
        return {};
    }

    std::string log(static_cast<std::size_t>(log_length), '\0');
    GLsizei written = 0;
    glGetProgramInfoLog(program, log_length, &written, log.data());
    log.resize(static_cast<std::size_t>(written));
    return log;
}

[[nodiscard]] ShaderObject compile_shader(const ShaderStageSource& stage) {
    ShaderObject shader{glCreateShader(std::to_underlying(stage.stage))};
    if (shader.id() == 0) {
        throw std::runtime_error{"OpenGL failed to create the " +
                                 std::string{shader_stage_name(stage.stage)} + " shader object."};
    }

    const GLchar* source_data = stage.source.data();
    // Keep the OpenGL API width visible at this narrowing boundary.
    // NOLINTNEXTLINE(modernize-use-auto)
    const GLint source_length = static_cast<GLint>(stage.source.size());
    glShaderSource(shader.id(), 1, &source_data, &source_length);
    glCompileShader(shader.id());

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader.id(), GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        throw std::runtime_error{std::string{shader_stage_name(stage.stage)} +
                                 " shader compilation failed:\n" + shader_log(shader.id())};
    }

    return shader;
}

} // namespace

ShaderProgram::ShaderProgram(std::span<const ShaderStageSource> stages) {
    if (stages.empty()) {
        throw std::invalid_argument{"A shader program requires at least one stage."};
    }

    for (const ShaderStageSource& stage : stages) {
        if (stage.source.empty()) {
            throw std::invalid_argument{std::string{shader_stage_name(stage.stage)} +
                                        " shader source must not be empty."};
        }
    }

    std::vector<ShaderObject> shaders;
    shaders.reserve(stages.size());
    for (const ShaderStageSource& stage : stages) {
        shaders.push_back(compile_shader(stage));
    }

    try {
        program_ = glCreateProgram();
        if (program_ == 0) {
            throw std::runtime_error{"OpenGL failed to allocate a program object."};
        }

        for (const ShaderObject& shader : shaders) {
            glAttachShader(program_, shader.id());
        }
        glLinkProgram(program_);

        GLint linked = GL_FALSE;
        glGetProgramiv(program_, GL_LINK_STATUS, &linked);
        if (linked != GL_TRUE) {
            throw std::runtime_error{"Shader link failed:\n" + program_log(program_)};
        }
    } catch (...) {
        if (program_ != 0) {
            glDeleteProgram(program_);
            program_ = 0;
        }
        throw;
    }
}

ShaderProgram::~ShaderProgram() {
    if (program_ != 0) {
        glDeleteProgram(program_);
    }
}

GLuint ShaderProgram::id() const noexcept {
    return program_;
}

} // namespace gps
