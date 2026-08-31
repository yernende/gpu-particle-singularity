#pragma once

#include <glad/gl.h>
#include <span>
#include <string_view>

namespace gps {

// GL shader enum values exceed 16-bit range, so keep the enum aligned with GLenum
// and suppress the clang warning for the raw GL_* initializer values.
// NOLINTBEGIN(performance-enum-size)
enum class ShaderStage : GLenum {
    vertex = GL_VERTEX_SHADER,
    tessellation_control = GL_TESS_CONTROL_SHADER,
    tessellation_evaluation = GL_TESS_EVALUATION_SHADER,
    geometry = GL_GEOMETRY_SHADER,
    fragment = GL_FRAGMENT_SHADER,
    compute = GL_COMPUTE_SHADER,
};
// NOLINTEND(performance-enum-size)

struct ShaderStageSource {
    ShaderStage stage;
    std::string_view source;
};

// A current OpenGL context must outlive every shader program.
class ShaderProgram final {
  public:
    explicit ShaderProgram(std::span<const ShaderStageSource> stages);
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&&) = delete;
    ShaderProgram& operator=(ShaderProgram&&) = delete;

    [[nodiscard]] GLuint id() const noexcept;

  private:
    GLuint program_{0};
};

} // namespace gps
