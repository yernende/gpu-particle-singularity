# GPU Particle Singularity

## How to use this roadmap

This roadmap is divided into **vertical features**, not tiny implementation chores. Each numbered
feature should leave the application runnable, demonstrable, and suitable for one coherent commit.
The many checkboxes inside a feature are progress markers; they are not suggestions to create one
commit per checkbox.

For every task:

- [ ] Read and understand the affected code before changing it.
- [ ] Keep AI-generated code only when every line and OpenGL call is understood.
- [ ] Build and run after each meaningful internal step.
- [ ] Review the complete diff before finishing the feature.
- [ ] Remove abstractions, comments, and defensive code that cannot be explained.
- [ ] Complete the feature's acceptance check before making its commit.

The intended rhythm is:

```text
understand → implement → run → inspect → test → commit
```

---

# Definition of Done

The finished first version must provide all of the following:

- [ ] A branded `gpu-particle-singularity` C++23/OpenGL 4.6 application.
- [ ] At least `65,536` particles simulated on the GPU after initial CPU-side creation.
- [ ] Particle state stored in an SSBO and shared by compute and graphics shaders.
- [ ] A compute shader that updates position, velocity, age, lifetime, and respawning.
- [ ] A softened central attraction force, tangential vortex force, and velocity drag.
- [ ] A bounded fixed-step simulation that remains stable after pauses and frame-time spikes.
- [ ] Deterministic reset from a user-visible seed.
- [ ] Soft camera-facing particle billboards rendered with instancing.
- [ ] Additive particle blending without per-particle sorting.
- [ ] Color, opacity, and size variation over particle lifetime.
- [ ] An opaque central sphere that correctly occludes particles through the depth buffer.
- [ ] Dear ImGui controls for simulation, emitter, force, and rendering parameters.
- [ ] Runtime handling for unsupported particle counts and invalid parameter combinations.
- [ ] A smoke test that exercises a real compute dispatch and rendered frames.
- [ ] CPU-side tests for particle layout, deterministic initialization, validation, and dispatch math.
- [ ] Debug and Release builds on Windows x64.
- [ ] Debug and Release builds on Ubuntu 24.04 x64 through CI or a local machine.
- [ ] No OpenGL debug errors during normal use or smoke testing.
- [ ] A README with a screenshot or short animation and an explanation of the complete data flow.

The first version intentionally does **not** include:

- particle-to-particle forces;
- N-body gravity;
- general collision detection;
- physically accurate black-hole or relativistic physics;
- ordinary alpha blending that requires depth sorting;
- order-independent transparency;
- textures or texture atlases;
- HDR rendering or bloom;
- motion-vector trails;
- transform feedback;
- indirect drawing;
- persistent mapped buffers;
- shader hot reload;
- an orbit camera;
- an ECS, material system, render graph, or custom engine framework;
- Vulkan, CUDA, or another compute API.

Record those ideas under `Future work`. They are not reasons to delay the first finished particle
system.

---

# Technical Baseline

Keep the starter's existing platform and tool choices:

- C++23;
- OpenGL 4.6 Core;
- GLSL 4.60;
- GLFW;
- GLAD2;
- GLM;
- Dear ImGui;
- CMake and Ninja;
- vcpkg in manifest mode;
- Catch2 and CTest;
- Windows x64 with MSVC;
- Ubuntu 24.04 x64 with GCC or Clang;
- macOS explicitly unsupported.

Recommended initial constants:

```text
Initial debug particle count:       4,096
Finished default particle count:   65,536
Initial random seed:                12,345
Initial orbital speed:                0.35
Compute local size X:                 256
SSBO binding index:                     0
Fixed simulation step:             1 / 120 s
Maximum substeps per frame:               8
Maximum accepted frame delta:          0.25 s
Core radius:                            0.45
Emitter inner radius:                   1.40
Emitter outer radius:                   3.20
Emitter half-thickness:                 0.20
Escape radius:                          8.00
Lifetime range:                      6–18 s
```

These are starting values, not sacred physical constants. Tune them only after the complete force
field works.

Keep shader source embedded as `constexpr std::string_view` values for this project. External
shader files would add path discovery and deployment behavior without teaching anything essential
about particles.

---

# Sequential Feature Roadmap

## Feature 1: Establish a branded, verified project baseline

**User-visible result:** the starter builds and runs as `gpu-particle-singularity`, while the
triangle remains temporarily as proof that the renamed baseline still works.

**Suggested commit:** `Prepare the GPU particle singularity project`

### Rename the project consistently

Use these four identity forms:

```text
gpu-particle-singularity   repository, executable, package, and user-facing name
gpu_particle_singularity   CMake project and target name
gps                        C++ namespace and CMake helper prefix
GPS                        CMake option and internal variable prefix
```

- [x] Build and run the untouched starter before renaming anything.
- [x] Run the existing Catch2 tests before renaming anything.
- [x] Run the existing `--smoke-test` before renaming anything.
- [x] Rename the CMake project and executable target.
- [x] Set the executable output name to `gpu-particle-singularity`.
- [x] Update the CMake project description.
- [x] Rename `olb_configure_target` and related CMake helper names to the `gps` prefix.
- [x] Rename every first-party `OLB_*` option and variable to `GPS_*`.
- [x] Rename the C++ namespace from `olb` to `gps`.
- [x] Update the window title.
- [x] Update `--help` usage text and error messages.
- [x] Update CMake presets where target or option names appear.
- [x] Update GitHub Actions commands and target names.
- [x] Update VS Code launch and task configuration.
- [x] Update the README title and introductory paragraph.
- [x] Keep `third_party/glad` unchanged.
- [x] Keep the temporary `TriangleDemo` working.

Search for old identity forms:

```powershell
rg -n --glob '!third_party/glad/**' \
  'opengl-lesson-bootstrap|opengl_lesson_bootstrap|\bolb\b|OLB' .
```

- [x] Confirm that the search returns no unintended old project identity.
- [x] Configure and build the Windows Debug preset.
- [x] Configure and build the Windows Release preset.
- [x] Run CTest.
- [x] Run `--smoke-test`.
- [x] Confirm that a shader failure still prints its compilation or linking log.
- [x] Confirm that OpenGL objects are destroyed before the GLFW context is destroyed.

### Understanding check

- [x] Explain why project renaming must include CMake, source code, CI, and editor configuration.
- [x] Explain why `third_party/glad` should not be modified during an application rename.
- [x] Explain why OpenGL-owning objects must die while their context is still current.
- [x] Explain the current frame order from event polling through buffer swap.

### Acceptance check

```text
1. The executable is named gpu-particle-singularity.
2. The window opens and still renders the temporary triangle.
3. --help, tests, and --smoke-test succeed.
4. No first-party olb / OLB identity remains.
```

---

## Feature 2: Render a static particle field from an SSBO

**User-visible result:** the triangle is replaced by thousands of stationary particles arranged in
an annular cloud around the origin. The graphics shader reads particle state from a Shader Storage
Buffer Object.

**Suggested commit:** `Render a static particle field from an SSBO`

### Define the CPU/GPU particle layout

Use one explicit 48-byte representation shared conceptually by C++ and GLSL:

```cpp
struct ParticleGpu {
    glm::vec4 position_age;
    glm::vec4 velocity_lifetime;
    glm::uvec4 random_state;
};
```

Interpret the fields as:

```text
position_age.xyz       world-space position
position_age.w         age in seconds
velocity_lifetime.xyz  world-space velocity
velocity_lifetime.w    lifetime in seconds
random_state.x         nonzero persistent random state
random_state.yzw       reserved and initialized to zero
```

The matching GLSL layout is:

```glsl
struct Particle {
    vec4 positionAge;
    vec4 velocityLifetime;
    uvec4 randomState;
};

layout(std430, binding = 0) readonly buffer ParticleBuffer {
    Particle particles[];
};
```

- [x] Add `ParticleGpu` in a small header with no OpenGL object ownership.
- [x] Use only fixed-width integer types where C++ data crosses into GLSL.
- [x] Do not place C++ `bool` values in the shared structure.
- [x] Add `static_assert(std::is_standard_layout_v<ParticleGpu>)`.
- [x] Add `static_assert(std::is_trivially_copyable_v<ParticleGpu>)`.
- [x] ~Add `static_assert(alignof(ParticleGpu) == 16)`~. I decided that I don't need this
- [x] Add `static_assert(sizeof(ParticleGpu) == 48)`.
- [x] Assert that the three field offsets are `0`, `16`, and `32` bytes.
- [ ] Add a Catch2 test that documents the expected layout.

The explicit checks matter because `std430` and C++ must agree. A layout mismatch usually produces
plausible-looking garbage, which is the GPU's preferred form of emotional abuse.

### Generate a deterministic annular cloud on the CPU

Let `u1` through `u5` be successive pseudo-random values in `[0, 1)`. The first three values
determine position, while `u4` and `u5` determine lifetime and initial age.

Sample the angle:

$$
\phi = 2\pi u_1
$$

Sample radius uniformly by **area**, not uniformly by radius:

$$
r =
\sqrt{
    R_{\min}^{2}
    + u_2
    \left(
        R_{\max}^{2} - R_{\min}^{2}
    \right)
}
$$

Sample vertical position inside a thin disk:

$$
y = H(2u_3 - 1)
$$

Construct the position:

$$
\mathbf p =
\begin{bmatrix}
    r\cos\phi \\
    y \\
    r\sin\phi
\end{bmatrix}
$$

A convenient tangential direction around the world Y axis is:

$$
\mathbf t =
\begin{bmatrix}
    -\sin\phi \\
    0 \\
    \cos\phi
\end{bmatrix}
$$

Use it for an initial velocity even though particles remain static in this feature:

$$
\mathbf v_0 = v_{\text{orbit}}\mathbf t
$$

Sample lifetime and stagger the initial age:

$$
L = L_{\min} + u_4(L_{\max} - L_{\min})
$$

$$
a = L u_5
$$

One `Xorshift32` stream generates the complete array. A zero user seed is valid: it is replaced by
a fixed nonzero internal state because zero is an absorbing state for xorshift32. After generating
each particle, the current nonzero state is stored in `random_state.x` for future GPU respawning.

- [x] Add a small deterministic 32-bit random generator or hash-based generator.
- [x] Guarantee that every stored random state is nonzero.
- [x] Accept a zero user seed by replacing the RNG's initial state with a fixed nonzero fallback.
- [x] Convert random integers to `[0, 1)` without `std::uniform_real_distribution` so the random
      sequence itself is repeatable across standard-library implementations.

Exact bitwise equality of final positions across operating systems is not required because
trigonometric implementations may differ slightly. `Deterministic reset` here means that the same
build, settings, and seed reproduce the same initial state.
- [x] Add an initialization settings structure with particle count, seed, radii, thickness,
      lifetime range, and initial orbital speed.
- [x] Reject an inner radius less than or equal to zero.
- [x] Reject an outer radius smaller than the inner radius.
- [x] Reject a negative half-thickness.
- [x] Reject a non-positive minimum lifetime.
- [x] Reject a maximum lifetime smaller than the minimum lifetime.
- [x] Reject non-finite radii, thickness, lifetime bounds, and initial orbital speed.
- [x] Generate exactly the requested number of particles.
- [x] Treat a requested count of zero as a valid empty result.
- [x] Initialize every position inside the requested annulus and vertical range.
- [x] Initialize every velocity as a finite tangential vector around the world Y axis.
- [x] Initialize lifetime inside the requested range.
- [x] Stagger initial age in `[0, lifetime)` so the future emitter does not respawn all particles
      simultaneously.
- [x] Initialize `random_state.yzw` to zero.
- [x] Generate the CPU array while constructing `GpsDemo`; after `ParticleBuffer` copies its bytes
      into immutable storage, do not retain the temporary CPU vector.
- [x] Add a test that the same settings and seed produce the same particle fields.
- [x] Add a test that a different seed changes at least one particle.
- [x] Add tests for exact count, radius, thickness, tangential velocity, age, lifetime, finiteness,
      nonzero persistent state, and zeroed reserved state.

### Upload the particle array to an SSBO

Required byte count:

$$
B = N \cdot \operatorname{sizeof}(\texttt{ParticleGpu})
$$

For `65,536` particles and a `48`-byte structure:

$$
B = 65{,}536 \cdot 48 = 3{,}145{,}728\ \text{bytes}
$$

That is exactly `3 MiB`.

- [x] Add a small RAII owner for the particle buffer.
- [x] Reject an empty array because OpenGL immutable buffer storage requires a positive byte count.
- [x] Create the buffer with `glCreateBuffers`.
- [x] Treat a returned object name of `0` as allocation failure.
- [x] Check multiplication for overflow before computing the byte count.
- [x] Check that the byte count fits `GLsizeiptr`.
- [x] Query `GL_MAX_SHADER_STORAGE_BLOCK_SIZE` with `glGetInteger64v`.
- [x] Reject a requested buffer larger than the implementation limit with a clear message.
- [x] Allocate immutable storage with `glNamedBufferStorage`.
- [x] Include `GL_DYNAMIC_STORAGE_BIT` so reset can later replace the contents with
      `glNamedBufferSubData` without reallocating storage.
- [x] Bind the object to SSBO binding index `0` with `glBindBufferBase`.
- [x] Delete the buffer in the RAII owner's destructor.
- [x] Delete copying for the owner.
- [x] Keep movement deleted and store the owner directly in `GpsDemo`.
- [x] Construct `GpsDemo` inside the OpenGL context's lifetime so its owner is also destroyed there.

### Render particles as diagnostic points

A vertex shader invocation can select a particle without a vertex attribute:

```glsl
uint particleIndex = uint(gl_VertexID);
Particle particle = particles[particleIndex];
```

- [x] Create and bind an otherwise empty VAO; a VAO is still required in the core profile.
- [x] Add a fixed model, view, and projection path.
- [x] Skip rendering when the framebuffer width or height is zero.
- [x] Read the particle by `gl_VertexID` in the vertex shader.
- [x] Transform `positionAge.xyz` with view and projection matrices.
- [x] Set a small diagnostic `gl_PointSize`.
- [x] Enable `GL_PROGRAM_POINT_SIZE`.
- [x] Use `gl_PointCoord` in the fragment shader.
- [x] Convert the square point primitive into a circular mark with a radial test.
- [x] Discard fragments outside the unit circle.
- [x] Keep points opaque for this feature; blending comes later.
- [x] Validate every required uniform location.
- [x] Draw with `glDrawArrays(GL_POINTS, 0, particleCount)`.
- [x] Check that `particleCount` fits `GLsizei` before drawing.
- [x] Remove `TriangleDemo` only after the static cloud renders successfully.
- [x] Remove the triangle files from CMake.
- [x] Keep the old triangle commit available as a working reference in Git history.

### Understanding check

- [x] Explain the difference between a VBO used as vertex attributes and an SSBO read manually.
- [x] Explain what `layout(std430)` controls.
- [x] Explain why C++ and GLSL field offsets must match exactly.
- [x] Explain why an empty VAO is required even though no vertex attributes are used.
- [x] Explain how `gl_VertexID` maps one draw vertex to one particle.
- [x] Explain why uniform-by-area annulus sampling uses a square root.

### Acceptance check

```text
1. The triangle is gone.
2. At least 4,096 stationary circular points form a thin annular cloud.
3. The same seed reproduces the same cloud after restarting the application.
4. CPU tests validate layout and initialization.
5. No compute shader exists yet.
```

---

## Feature 3: Move particles with a compute shader

**User-visible result:** all particles move according to their stored velocities, and the CPU no
longer updates individual particle positions.

**Suggested commit:** `Simulate particle motion with a compute shader`

### Generalize shader-program construction

The starter's shader wrapper currently assumes exactly one vertex shader and one fragment shader.
Represent the finite set of OpenGL shader stages with a scoped enum, then pair a stage with its
source explicitly:

```cpp
enum class ShaderStage : GLenum {
    vertex = GL_VERTEX_SHADER,
    tessellation_control = GL_TESS_CONTROL_SHADER,
    tessellation_evaluation = GL_TESS_EVALUATION_SHADER,
    geometry = GL_GEOMETRY_SHADER,
    fragment = GL_FRAGMENT_SHADER,
    compute = GL_COMPUTE_SHADER,
};

struct ShaderStageSource {
    ShaderStage stage;
    std::string_view source;
};
```

Construct a program from a span of stages:

```cpp
ShaderProgram(std::span<const ShaderStageSource> stages);
```

- [x] Use a scoped enum for the valid OpenGL shader stages.
- [x] Preserve the existing RAII ownership of the OpenGL program.
- [x] Reject an empty stage list.
- [x] Reject an empty shader source.
- [x] Compile every supplied stage separately.
- [x] Include a readable stage name in compilation errors.
- [x] Attach all successfully compiled shaders.
- [x] Link exactly once.
- [x] Include the complete program log in linking errors.
- [x] Delete every temporary shader after successful linking.
- [x] Delete every already-created shader if a later compilation fails.
- [x] Delete the program if linking fails.
- [x] Keep copying and movement intentionally disabled unless correct movement is needed.
- [x] Update the graphics program call site to pass vertex and fragment stages.
- [x] Do not add shader reflection, uniform caching, includes, or hot reload.

### Add the first compute shader

- [x] Add the compute program call site with one compute stage.

Use a one-dimensional work group:

```glsl
layout(local_size_x = 256) in;
```

One invocation updates one particle:

```glsl
uint index = gl_GlobalInvocationID.x;

if (index >= uParticleCount) {
    return;
}

particles[index].positionAge.xyz +=
    particles[index].velocityLifetime.xyz * uDeltaTime;
```

- [x] Declare the particle SSBO as writable in the compute shader.
- [x] Use the same explicit binding index as the rendering shader.
- [x] Add `uParticleCount` as an unsigned integer uniform.
- [x] Add `uDeltaTime` as a float uniform.
- [x] Guard every invocation whose global index is outside the active particle count.
- [x] Do not read or write another particle from an invocation.
- [x] Validate all required compute uniform locations.

### Dispatch enough work groups

For particle count `N` and local size `L`:

$$
G =
\left\lceil
    \frac{N}{L}
\right\rceil
$$

A robust integer implementation avoids overflow:

```cpp
const auto group_count =
    particle_count / local_size +
    static_cast<unsigned>(particle_count % local_size != 0);
```

Examples for `L = 256`:

| Particles | Work groups | Total invocations |
|----------:|------------:|------------------:|
| 1 | 1 | 256 |
| 255 | 1 | 256 |
| 256 | 1 | 256 |
| 257 | 2 | 512 |
| 65,536 | 256 | 65,536 |
| 65,537 | 257 | 65,792 |

- [x] Add a small dispatch-count helper that rejects a zero local size.
- [x] Add tests for `0`, `1`, `255`, `256`, `257`, `65,536`, and `65,537`.
- [x] Bind the particle SSBO before dispatch.
- [x] Set compute uniforms before dispatch.
- [x] Call `glDispatchCompute(groupCount, 1, 1)`.
- [x] Immediately call `glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT)` after dispatch.
- [x] Render from the same SSBO after the barrier.
- [x] Use `4,097` particles by default so regular and smoke runs exercise a partial work group.
- [x] Keep the bounds guard before the compute shader's first particle-buffer access.

For this design, `GL_SHADER_STORAGE_BARRIER_BIT` is the relevant barrier because both the compute
shader and vertex shader access the buffer through shader-storage blocks.

The caller supplies a bounded wall-clock delta. This keeps dispatch and the temporary timing policy
as separate learning steps while leaving the fixed-step accumulator for Feature 5.

### Use a bounded temporary frame delta

This feature may still use one simulation dispatch per rendered frame. Keep it safe enough for the
intermediate state:

```cpp
const double frame_delta =
    std::clamp(now - previous_time, 0.0, 1.0 / 30.0);
```

- [x] Update `previous_time` before any framebuffer-size or future pause gate, so minimized or
      paused frames cannot accumulate elapsed time.
- [x] Clamp negative deltas to zero.
- [x] Clamp abnormally large deltas to `1 / 30` second.
- [x] Do not use frame number as simulated time.
- [x] Keep the final fixed-step accumulator for Feature 5.

### Understanding check

- [x] Explain the difference between dispatching a compute program and issuing a draw call.
- [x] Explain local invocation ID, work-group ID, and global invocation ID.
- [x] Explain why extra invocations are normal when the count is not divisible by the local size.
- [x] Explain why the bounds guard belongs inside the compute shader.
- [x] Explain what data hazard the memory barrier resolves.
- [x] Explain why no ping-pong buffer is required while each invocation accesses only its own particle.

### Acceptance check

```text
1. The CPU uploads the initial state once.
2. The compute shader changes positions every frame.
3. The point renderer displays the updated state from the same SSBO.
4. Counts such as 257 and 65,537 work without OpenGL errors.
5. Particles currently drift away because lifecycle and forces are not implemented yet.
```

---

## Feature 4: Turn the moving cloud into a persistent emitter

**User-visible result:** particles age, die, and respawn on the annulus indefinitely. The scene no
longer empties as particles leave the camera.

**Suggested commit:** `Add particle lifetime and deterministic respawning`

### Maintain particle age and lifetime

Normalized lifetime progress is:

$$
q =
\operatorname{clamp}
\left(
    \frac{a}{T},
    0,
    1
\right)
$$

Where:

- $a$ is current age;
- $T$ is lifetime;
- $q=0$ means newly born;
- $q=1$ means expired.

- [ ] Increment age by `uDeltaTime` in the compute shader.
- [ ] Treat a non-positive lifetime as invalid state that must respawn.
- [ ] Respawn when `age >= lifetime`.
- [ ] Respawn when distance from the origin exceeds the escape radius.
- [ ] Keep core capture for Feature 5, when the attraction force exists.
- [ ] Preserve finite position and velocity for every live particle.

### Add a persistent GPU random state

A compact xorshift32 step is sufficient for visual particle emission:

```glsl
uint nextUint(inout uint state) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}
```

Convert the upper 24 bits to `[0, 1)`:

```glsl
float nextUnitFloat(inout uint state) {
    return float(nextUint(state) >> 8) * (1.0 / 16777216.0);
}
```

A zero xorshift state remains zero forever, so it is forbidden.

- [ ] Initialize `randomState.x` from particle index and the global seed.
- [ ] Replace a generated zero state with a fixed nonzero fallback.
- [ ] Load the state into a local compute-shader variable before generating values.
- [ ] Advance the state for every emitted random value.
- [ ] Store the updated state back into the particle after respawn.
- [ ] Confirm that different particles do not all use the same random sequence.
- [ ] Keep random state changes on the GPU; do not read them back every frame.

### Implement a compute-shader respawn function

Use the same annulus equations as CPU initialization:

$$
\phi = 2\pi u_1
$$

$$
r =
\sqrt{
    R_{\min}^{2}
    + u_2
    \left(
        R_{\max}^{2} - R_{\min}^{2}
    \right)
}
$$

$$
\mathbf p =
\begin{bmatrix}
    r\cos\phi \\
    H(2u_3 - 1) \\
    r\sin\phi
\end{bmatrix}
$$

$$
\mathbf v =
v_{\text{orbit}}
\begin{bmatrix}
    -\sin\phi \\
    0 \\
    \cos\phi
\end{bmatrix}
+
\mathbf v_{\text{jitter}}
$$

- [ ] Add uniforms for emitter inner radius, outer radius, and half-thickness.
- [ ] Add uniforms for minimum and maximum lifetime.
- [ ] Add uniforms for orbital speed and velocity jitter.
- [ ] Add a respawn function that receives the particle's random state by `inout`.
- [ ] Set position from the annular emitter.
- [ ] Set velocity from tangential motion plus bounded jitter.
- [ ] Reset age to zero.
- [ ] Choose a new lifetime inside the configured range.
- [ ] Store the advanced random state.
- [ ] Respawn and stop processing that particle for the current simulation step.
- [ ] Avoid normalizing any vector that may have zero length.

### Recover corrupted numerical state

```glsl
bool finiteVec3(vec3 value) {
    return
        !any(isnan(value)) &&
        !any(isinf(value));
}
```

- [ ] Respawn when position is non-finite.
- [ ] Respawn when velocity is non-finite.
- [ ] Respawn when age or lifetime is NaN or infinite.
- [ ] Keep this recovery even after the force equations appear stable.

### Add deterministic reset

- [ ] Add a temporary ImGui `Reset particles` button.
- [ ] Re-run CPU initialization from the current global seed when reset is pressed.
- [ ] Upload replacement state with `glNamedBufferSubData`.
- [ ] Reset timing state so reset does not inherit a large frame delta.
- [ ] Confirm that the same seed recreates the same initial cloud.
- [ ] Confirm that changing the seed creates a visibly different cloud.
- [ ] Do not recreate the OpenGL buffer merely to reset its contents.

### Understanding check

- [ ] Explain why pseudo-random generators carry mutable state.
- [ ] Explain why xorshift32 cannot recover from a zero state.
- [ ] Explain why lifetime is stored per particle rather than only as a global uniform.
- [ ] Explain why reset uploads state but ordinary frames do not.
- [ ] Explain why numerical recovery is useful even when inputs are validated on the CPU.

### Acceptance check

```text
1. Particles continuously respawn instead of permanently escaping.
2. Initial ages are staggered, so respawning is not synchronized.
3. Reset with the same seed reproduces the same initial field.
4. No per-frame GPU-to-CPU readback occurs.
5. The motion is still simple linear velocity; the singularity force comes next.
```

---

## Feature 5: Implement the singularity force field and stable integration

**User-visible result:** particles orbit, spiral inward, disappear into the core, and respawn from
the outer emitter.

**Suggested commit:** `Implement the singularity particle dynamics`

### Add a softened central attraction

Let:

- $\mathbf p$ be particle position relative to the origin;
- $r^2 = \mathbf p \cdot \mathbf p$;
- $G$ be attraction strength;
- $\varepsilon$ be a softening distance.

Use:

$$
\mathbf a_{\text{attraction}}
=
-G
\frac{\mathbf p}
{\left(r^2 + \varepsilon^2\right)^{3/2}}
$$

Without softening, inverse-square acceleration becomes unbounded near the origin. The project is
called a singularity; the floating-point representation does not need to become one.

GLSL form:

```glsl
float softenedRadiusSquared =
    dot(position, position) +
    uSoftening * uSoftening;

float inverseRadius =
    inversesqrt(softenedRadiusSquared);

float inverseRadiusCubed =
    inverseRadius * inverseRadius * inverseRadius;

vec3 attraction =
    -uAttractionStrength *
    position *
    inverseRadiusCubed;
```

- [ ] Add positive attraction-strength and softening uniforms.
- [ ] Validate a strictly positive softening value on the CPU.
- [ ] Compute attraction without calling `normalize(position)`.
- [ ] Keep the sign directed toward the origin.
- [ ] Test attraction alone before adding other forces.
- [ ] Confirm that particles accelerate inward.

### Add tangential vortex acceleration

Project the position onto the XZ plane:

$$
\mathbf r_{\perp} =
\begin{bmatrix}
x \\
0 \\
z
\end{bmatrix}
$$

Using world up $\hat{\mathbf y}$, a tangent is:

$$
\mathbf t =
\frac{
    \hat{\mathbf y} \times \mathbf r_{\perp}
}{
    \left\lVert
        \hat{\mathbf y} \times \mathbf r_{\perp}
    \right\rVert
}
$$

Then:

$$
\mathbf a_{\text{vortex}} = S\mathbf t
$$

Where $S$ is swirl strength.

- [ ] Add a signed swirl-strength uniform.
- [ ] Compute the unnormalized tangent with a cross product.
- [ ] Check tangent length before normalization.
- [ ] Use zero vortex acceleration near the Y axis instead of normalizing zero.
- [ ] Allow the swirl sign to reverse rotation direction.
- [ ] Test vortex acceleration without attraction.
- [ ] Restore attraction and confirm the two effects combine.

### Add velocity drag

Use linear drag:

$$
\mathbf a_{\text{drag}} = -D\mathbf v
$$

Where $D \geq 0$ is the drag coefficient.

Total acceleration:

$$
\mathbf a =
\mathbf a_{\text{attraction}}
+
\mathbf a_{\text{vortex}}
+
\mathbf a_{\text{drag}}
$$

- [ ] Add a non-negative drag uniform.
- [ ] Add all acceleration terms before integration.
- [ ] Optionally clamp acceleration magnitude to a configurable safety maximum.
- [ ] Keep the acceleration clamp disabled or generous under normal defaults.
- [ ] Confirm that drag causes orbits to decay gradually instead of instantly stopping particles.

### Integrate with semi-implicit Euler

Use:

$$
\mathbf v_{n+1}
=
\mathbf v_n
+
\mathbf a_n\Delta t
$$

Then:

$$
\mathbf p_{n+1}
=
\mathbf p_n
+
\mathbf v_{n+1}\Delta t
$$

Updating position with the new velocity is semi-implicit Euler. It is still simple, but generally
behaves better for orbital motion than updating position from the old velocity.

- [ ] Update velocity first.
- [ ] Update position from the new velocity.
- [ ] Update age using the same simulation step.
- [ ] Capture and respawn particles whose radius is smaller than the core radius.
- [ ] Continue respawning particles outside the escape radius.
- [ ] Run finite-state recovery after force integration as well as before it.
- [ ] Verify that every force is multiplied by time through the integration equations, not baked
      into frame-dependent constants.

### Replace frame-dependent stepping with a fixed-step accumulator

Let fixed simulation step be:

$$
h = \frac{1}{120}\ \text{second}
$$

At each rendered frame:

$$
A \leftarrow
\min
\left(
    A + \Delta t_{\text{frame}},
    A_{\max}
\right)
$$

While enough accumulated time remains:

$$
A \geq h
$$

perform one compute dispatch with `uDeltaTime = h`, then:

$$
A \leftarrow A - h
$$

- [ ] Store an accumulator in double precision on the CPU.
- [ ] Use a fixed step of `1 / 120` second by default.
- [ ] Clamp incoming frame delta to at most `0.25` second.
- [ ] Limit simulation to at most `8` substeps per rendered frame.
- [ ] Define what happens to excess accumulated time after the substep cap; dropping it is acceptable.
- [ ] Call the memory barrier after every dispatch, including between consecutive substeps.
- [ ] Keep rendering once per display frame, not once per simulation substep.
- [ ] Do not accumulate time while paused.
- [ ] Update the previous wall-clock time even while paused.
- [ ] Clear the accumulator when particles are reset.
- [ ] Add a one-fixed-step path for a future `Single step` button.
- [ ] Confirm that dragging the window or stopping at a debugger breakpoint does not launch particles
      into another dimension.

### Tune a first coherent parameter set

Start around:

```text
Attraction strength:  3.0
Softening:            0.30
Swirl strength:       0.45
Drag:                 0.08
Initial orbit speed:  1.25
Core radius:          0.45
Escape radius:        8.00
```

- [ ] Tune attraction alone.
- [ ] Tune initial orbital speed with attraction.
- [ ] Add swirl and tune its sign and magnitude.
- [ ] Add drag last.
- [ ] Confirm that a substantial fraction of particles eventually reach the core.
- [ ] Confirm that particles do not all collapse immediately.
- [ ] Confirm that the emitter remains visibly populated.
- [ ] Keep defaults visually useful at both `4,096` and `65,536` particles.

### Understanding check

- [ ] Explain why the attraction denominator has power `3/2` when the numerator is the position vector.
- [ ] Explain what softening changes near the origin.
- [ ] Explain how a cross product creates the vortex tangent.
- [ ] Explain the difference between explicit and semi-implicit Euler order.
- [ ] Explain why fixed simulation steps reduce frame-rate dependence.
- [ ] Explain why a memory barrier is required between two compute substeps using the same SSBO.

### Acceptance check

```text
1. Particles visibly orbit and spiral toward the origin.
2. Particles entering the core radius respawn at the emitter.
3. Pausing, moving the window, or hitting a breakpoint does not destabilize the simulation.
4. Simulation behavior remains broadly similar at different rendering frame rates.
5. No particle-to-particle interaction is implemented.
```

---

## Feature 6: Turn the demo into an interactive particle laboratory

**User-visible result:** the singularity can be explored through structured ImGui controls without
editing source code or recompiling.

**Suggested commit:** `Add interactive particle simulation controls`

### Separate live settings from reset-only settings

Recommended categories:

```text
Live force settings
- attraction strength
- softening
- swirl strength
- drag
- optional acceleration limit

Live rendering settings
- point size for the current diagnostic renderer
- colors later used by the billboard renderer

Respawn settings
- emitter radii
- emitter thickness
- orbital speed
- velocity jitter
- lifetime range

Recreate settings
- particle count
- seed
```

Emitter changes naturally affect newly respawned particles. Use `Reset particles` when the user
wants those settings applied to the entire current population immediately.

- [ ] Create one value-type settings structure with deliberate defaults.
- [ ] Keep GPU object names out of the settings structure.
- [ ] Distinguish active particle count from a pending UI count.
- [ ] Recreate or resize the buffer only after an explicit `Apply particle count` action.
- [ ] Do not allocate GPU memory on every slider movement.
- [ ] Apply force settings immediately through compute uniforms.
- [ ] Apply rendering settings immediately through graphics uniforms.
- [ ] Apply emitter settings to future respawns.
- [ ] Document that `Reset particles` applies emitter changes to all particles immediately.

### Validate parameter relationships

Required invariants:

$$
0 < R_{\text{core}} < R_{\min} \leq R_{\max} < R_{\text{escape}}
$$

$$
0 < T_{\min} \leq T_{\max}
$$

$$
\varepsilon > 0
$$

$$
D \geq 0
$$

- [ ] Keep core radius positive.
- [ ] Keep emitter inner radius larger than core radius plus a small margin.
- [ ] Keep emitter outer radius at least as large as the inner radius.
- [ ] Keep escape radius larger than the emitter outer radius.
- [ ] Keep minimum lifetime positive.
- [ ] Keep maximum lifetime at least the minimum lifetime.
- [ ] Keep softening strictly positive.
- [ ] Keep drag non-negative.
- [ ] Keep particle count within the runtime-derived supported range.
- [ ] Reject or clamp invalid values before sending them to shaders.
- [ ] Display a clear validation message instead of silently creating broken state.
- [ ] Add CPU tests for valid defaults and invalid boundary combinations.

### Build a structured ImGui panel

- [ ] Add a `Simulation` section.
- [ ] Add `Pause`.
- [ ] Add `Single step`, enabled only while paused.
- [ ] Add `Reset particles`.
- [ ] Add `Reset parameters` separately from particle reset.
- [ ] Add a seed input.
- [ ] Add current fixed step and last substep count as read-only information.
- [ ] Add a `Particles` section with active count and pending count.
- [ ] Add a deliberate `Apply particle count` button.
- [ ] Add an `Emitter` section.
- [ ] Add a `Forces` section.
- [ ] Add a `Rendering` section.
- [ ] Add concise tooltips where a control's effect is not immediate or obvious.
- [ ] Keep labels stable so screenshots and documentation remain understandable.
- [ ] Avoid exposing every internal constant merely because it exists.

### Make reset behavior explicit and reliable

- [ ] `Reset particles` must preserve current parameters.
- [ ] `Reset particles` must preserve pause state.
- [ ] `Reset particles` must recreate initial state from the visible seed.
- [ ] `Reset parameters` must restore documented default values.
- [ ] Decide whether `Reset parameters` also resets particles; if it does, label that behavior clearly.
- [ ] Clear the simulation accumulator after any particle-state replacement.
- [ ] Rebind the SSBO after buffer recreation.
- [ ] Refresh any cached particle-count uniform after recreation.
- [ ] Confirm that reset works while paused and while running.

### Display useful diagnostics

Compute dispatch count:

$$
G =
\left\lceil
    \frac{N}{256}
\right\rceil
$$

Approximate particle-buffer size in MiB:

$$
M =
\frac{
    N \cdot \operatorname{sizeof}(\texttt{ParticleGpu})
}{
    1024^2
}
$$

- [ ] Show active particle count.
- [ ] Show work-group count per substep.
- [ ] Show particle-buffer size in MiB.
- [ ] Show rendered FPS or CPU frame time.
- [ ] Show the number of simulation substeps used for the latest frame.
- [ ] Show whether the requested count was clamped by a hardware or application limit.
- [ ] Do not call `glGet*` every frame for values that never change; cache hardware limits at startup.

### Understanding check

- [ ] Explain why some settings are live while others require reset or buffer recreation.
- [ ] Explain why validation belongs on the CPU even though the shader has numerical recovery.
- [ ] Explain why a particle-count slider should not reallocate continuously.
- [ ] Explain the difference between resetting parameters and resetting simulation state.

### Acceptance check

```text
1. The user can pause, single-step, reset, and change the seed.
2. Forces can be tuned live.
3. Particle count changes only after an explicit apply action.
4. Invalid parameter relationships cannot reach the compute shader.
5. The application exposes useful counts and timing information without per-frame limit queries.
```

---

## Feature 7: Replace diagnostic points with soft instanced billboards

**User-visible result:** particles become smooth, glowing camera-facing discs with lifetime-based
color, opacity, and size.

**Suggested commit:** `Render particles as soft additive billboards`

### Draw one quad instance per particle

No quad VBO is required if the six corners are generated from `gl_VertexID`:

```glsl
const vec2 corners[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2(+1.0, -1.0),
    vec2(+1.0, +1.0),
    vec2(-1.0, -1.0),
    vec2(+1.0, +1.0),
    vec2(-1.0, +1.0)
);
```

Select the particle with:

```glsl
uint particleIndex = uint(gl_InstanceID);
```

- [ ] Keep or create one empty VAO for the procedural quad draw.
- [ ] Draw six vertices per instance with `glDrawArraysInstanced`.
- [ ] Check that particle count fits the draw command's `GLsizei` instance count.
- [ ] Read particle state from the SSBO using `gl_InstanceID`.
- [ ] Use `gl_VertexID` only for the six local quad corners.
- [ ] Pass local UV coordinates to the fragment shader.
- [ ] Keep a diagnostic point-rendering toggle until billboards are proven correct.

### Expand billboards in view space

Let the particle center in view space be:

$$
\mathbf c_v = V
\begin{bmatrix}
\mathbf p \\
1
\end{bmatrix}
$$

For local corner $\mathbf q=(q_x,q_y)$ and particle size $s$:

$$
\mathbf p_v =
\mathbf c_v
+
\begin{bmatrix}
sq_x \\
sq_y \\
0 \\
0
\end{bmatrix}
$$

Then:

$$
\mathbf p_{clip} = P\mathbf p_v
$$

Expanding in view space makes the quad camera-facing without explicitly passing camera-right and
camera-up vectors.

- [ ] Transform the particle center into view space.
- [ ] Offset only view-space X and Y by the local corner.
- [ ] Preserve the center's view-space Z value for all six vertices.
- [ ] Project the expanded position.
- [ ] Confirm that billboard size decreases naturally with distance under perspective projection.
- [ ] Confirm that rotating or moving the camera in temporary tests does not reveal flat quad angles.

### Calculate lifetime appearance

Normalized age:

$$
q =
\operatorname{clamp}
\left(
    \frac{a}{T},
    0,
    1
\right)
$$

A simple fade envelope:

$$
f_{\text{in}}
=
\operatorname{smoothstep}
\left(0,q_{\text{in}},q\right)
$$

$$
f_{\text{out}}
=
1-
\operatorname{smoothstep}
\left(q_{\text{out}},1,q\right)
$$

$$
f_{\text{life}} = f_{\text{in}}f_{\text{out}}
$$

Example size curve:

$$
s(q) =
s_0
\operatorname{mix}
\left(s_{\text{start}},s_{\text{end}},q\right)
$$

- [ ] Pass normalized age or enough state to calculate it in the graphics shaders.
- [ ] Fade particles in over a short initial interval.
- [ ] Fade particles out before expiration.
- [ ] Vary size over lifetime.
- [ ] Use at least a two-color gradient over lifetime.
- [ ] Optionally use a third midpoint color for a hotter visual center.
- [ ] Expose base size and colors in ImGui.
- [ ] Avoid storing derived color and size in the SSBO unless there is a demonstrated need.

### Create a procedural soft disc

For fragment UV in `[0,1]^2`:

$$
\rho =
2
\left\lVert
    \mathbf{uv} -
    \begin{bmatrix}
    1/2 \\
    1/2
    \end{bmatrix}
\right\rVert
$$

A soft radial mask:

$$
m =
1-
\operatorname{smoothstep}
\left(r_{\text{soft}},1,\rho\right)
$$

Final source alpha:

$$
\alpha = m f_{\text{life}}
$$

- [ ] Calculate distance from the UV center.
- [ ] Discard fragments clearly outside the unit circle.
- [ ] Use `smoothstep` near the edge.
- [ ] Multiply the radial mask by the lifetime fade.
- [ ] Avoid a hard square boundary.
- [ ] Tune the center so particles look luminous without bloom.

### Use additive blending with correct depth behavior

```cpp
glEnable(GL_BLEND);
glBlendEquation(GL_FUNC_ADD);
glBlendFunc(GL_SRC_ALPHA, GL_ONE);

glEnable(GL_DEPTH_TEST);
glDepthMask(GL_FALSE);
```

After drawing particles:

```cpp
glDepthMask(GL_TRUE);
glDisable(GL_BLEND);
```

- [ ] Keep depth testing enabled for particle rendering.
- [ ] Disable depth writes only for the particle pass.
- [ ] Enable additive blending only for the particle pass.
- [ ] Use `GL_SRC_ALPHA, GL_ONE`.
- [ ] Disable face culling for the procedural billboards.
- [ ] Restore depth writes after the particle pass.
- [ ] Restore blending state before ImGui rendering.
- [ ] Confirm that changing render order among particles does not produce obvious alpha-sorting artifacts.
- [ ] Document that additive blending is the reason sorting is intentionally omitted.

### Understanding check

- [ ] Explain the separate meanings of `gl_VertexID` and `gl_InstanceID` in this draw.
- [ ] Explain why view-space expansion creates a camera-facing billboard.
- [ ] Explain why particles keep depth testing but disable depth writing.
- [ ] Explain why additive blending avoids the usual back-to-front sorting requirement.
- [ ] Explain why a soft disc can be generated without a texture.

### Acceptance check

```text
1. Particles render as soft discs rather than square points.
2. Every particle is one instanced procedural quad.
3. Color, opacity, and size evolve over lifetime.
4. Additive blending creates bright concentrations without particle sorting.
5. Render state is restored before ImGui.
```

---

## Feature 8: Add an opaque singularity core and depth composition

**User-visible result:** a dark central sphere anchors the effect, and particles behind it are
correctly hidden while particles in front remain visible.

**Suggested commit:** `Add the singularity core and depth composition`

### Add a small sphere mesh

Reusing the already-understood icosphere generator from the previous project is acceptable. The
learning target here is composition with the particle pass, not proving a second time that an
icosahedron has twenty faces.

- [ ] Copy or reimplement the minimal `MeshData` representation.
- [ ] Copy or reimplement the icosphere generator with understood code only.
- [ ] Generate positions, normals, and indices.
- [ ] Use subdivision level `2` or `3`; the core does not need thousands of tiny triangles.
- [ ] Upload one VAO, VBO, and EBO with RAII ownership.
- [ ] Validate non-empty geometry and index ranges.
- [ ] Enable back-face culling for the core.
- [ ] Add or copy the existing icosphere count and validity tests.

Expected counts:

$$
V_n = 10\cdot 4^n + 2
$$

$$
F_n = 20\cdot 4^n
$$

| Subdivisions | Vertices | Triangles |
|-------------:|---------:|----------:|
| 0 | 12 | 20 |
| 1 | 42 | 80 |
| 2 | 162 | 320 |
| 3 | 642 | 1,280 |

### Render the opaque core first

Required frame order:

```text
1. Clear color and depth.
2. Run all compute substeps.
3. Render the opaque core with depth writes enabled.
4. Render additive particles with depth test enabled and depth writes disabled.
5. Restore state.
6. Render ImGui.
```

- [ ] Render the core before particles.
- [ ] Keep blending disabled for the core.
- [ ] Keep depth testing and depth writes enabled for the core.
- [ ] Scale the sphere to the same radius used by compute-shader core capture.
- [ ] Render particles afterward with depth writes disabled.
- [ ] Confirm that particles behind the sphere fail the depth test.
- [ ] Confirm that particles in front remain visible.
- [ ] Confirm that changing core radius updates simulation capture and rendered geometry together.

### Add restrained unlit core shading

A view-dependent rim can use:

$$
F =
\left(
    1-
    \operatorname{clamp}
    \left(
        \hat{\mathbf n}\cdot\hat{\mathbf v},
        0,
        1
    \right)
\right)^k
$$

Where:

- $\hat{\mathbf n}$ is the world- or view-space surface normal;
- $\hat{\mathbf v}$ points from the surface toward the camera;
- $k$ controls rim sharpness.

This resembles a Fresnel rim visually, but it is not a complete physically based Fresnel model.

- [ ] Render the core body nearly black rather than exactly identical to the background.
- [ ] Pass or calculate a correctly transformed normal.
- [ ] Calculate view direction in the same coordinate space as the normal.
- [ ] Add a subtle rim color related to the particle palette.
- [ ] Expose core color, rim strength, and rim exponent only if they are useful during tuning.
- [ ] Keep lighting, shadows, PBR, and gravitational lensing out of scope.

### Understanding check

- [ ] Explain why the opaque pass must write depth before particles are rendered.
- [ ] Explain why particle depth writes remain disabled even though they test against core depth.
- [ ] Explain why normal and view direction must be in the same coordinate space.
- [ ] Explain the difference between a stylized rim term and physically based Fresnel reflectance.

### Acceptance check

```text
1. A dark sphere occupies exactly the compute shader's capture radius.
2. Particles disappear into the core rather than visibly crossing its front surface.
3. Particles behind the core are occluded by the depth buffer.
4. The rim remains subtle enough that particles are still the primary effect.
```

---

## Feature 9: Harden limits, recreation, smoke testing, and failure behavior

**User-visible result:** particle counts can be scaled safely, invalid states fail clearly, and the
existing smoke-test path now exercises the actual compute-and-render pipeline.

**Suggested commit:** `Harden particle limits and runtime diagnostics`

### Query and cache relevant implementation limits

At startup, query:

```text
GL_MAX_SHADER_STORAGE_BLOCK_SIZE
GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS
GL_MAX_COMPUTE_WORK_GROUP_SIZE[0]
GL_MAX_COMPUTE_WORK_GROUP_COUNT[0]
```

Derive the maximum supported particle count from all relevant limits:

$$
N_{\text{buffer}}
=
\left\lfloor
    \frac{
        B_{\max}
    }{
        \operatorname{sizeof}(\texttt{ParticleGpu})
    }
\right\rfloor
$$

$$
N_{\text{dispatch}}
=
G_{\max,x}L_x
$$

$$
N_{\max}
=
\min
\left(
    N_{\text{buffer}},
    N_{\text{dispatch}},
    \operatorname{GLsizei}_{\max},
    N_{\text{application-cap}}
\right)
$$

A reasonable application cap for the first version is `1,000,000` particles even if the driver
advertises much larger theoretical limits.

- [ ] Query the SSBO-size limit with `glGetInteger64v`.
- [ ] Query indexed work-group count and size limits with `glGetIntegeri_v`.
- [ ] Query the total invocation limit with `glGetIntegerv`.
- [ ] Query each limit once after OpenGL initialization.
- [ ] Check that local size `256` does not exceed X-size or total-invocation limits.
- [ ] Fail at startup with a direct diagnostic if the chosen compute layout is unsupported.
- [ ] Calculate supported particle count with checked integer arithmetic.
- [ ] Include the draw command's `GLsizei` range.
- [ ] Include an explicit application cap.
- [ ] Cache the result.
- [ ] Display the result in the diagnostics panel.
- [ ] Reject an unsupported count before allocating or dispatching.

### Make particle-buffer recreation predictable

- [ ] Validate the requested count before destroying existing state.
- [ ] Generate replacement CPU data before modifying the active GPU buffer.
- [ ] Keep old state intact if CPU initialization throws.
- [ ] Reallocate only when count actually changes.
- [ ] Re-upload without reallocation when only the seed or emitter state changes.
- [ ] Rebind binding index `0` after any buffer object replacement.
- [ ] Update active count only after successful creation.
- [ ] Clear timing accumulation after recreation.
- [ ] Keep a useful error message if OpenGL allocation fails.
- [ ] Confirm that repeated count changes do not leak buffer objects.

### Exercise boundary counts

- [ ] Run with `1` particle.
- [ ] Run with `255` particles.
- [ ] Run with `256` particles.
- [ ] Run with `257` particles.
- [ ] Run with `4,096` particles.
- [ ] Run with `65,536` particles.
- [ ] Run with `65,537` particles.
- [ ] Run with a high but supported count such as `262,144`.
- [ ] Attempt one count above the application cap and verify a clean rejection.
- [ ] Confirm that no count produces an out-of-bounds compute access or draw conversion.

### Upgrade the smoke test

The smoke test should prove more than “a window survived for three swaps.” It does not need a full
image-comparison bureaucracy.

- [ ] Keep smoke mode headless only in the sense of a hidden or non-interactive window; it still
      requires a real OpenGL context.
- [ ] Disable VSync in smoke mode.
- [ ] Use deterministic default settings and seed.
- [ ] Run at least one compute dispatch before the first validated frame.
- [ ] Render the core and billboards.
- [ ] Keep `glFinish` restricted to smoke mode.
- [ ] Check `glGetError` after finished smoke frames.
- [ ] Verify that the OpenGL debug callback reported no high-severity error.
- [ ] Optionally read back one particle only in smoke mode and verify that age or position changed.
- [ ] If reading shader-written data with `glGetNamedBufferSubData`, issue
      `glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT)` after the shader write and before the read.
- [ ] Keep the resulting synchronization and readback stall out of interactive mode.
- [ ] Exit nonzero when simulation or rendering validation fails.
- [ ] Keep invalid command-line arguments at exit code `2` and runtime failures at exit code `1`.

### Verify lifecycle and state restoration

- [ ] Minimize the window and restore it.
- [ ] Resize repeatedly, including very narrow and very short dimensions.
- [ ] Pause for several seconds and resume.
- [ ] Stop at a debugger breakpoint and resume.
- [ ] Reset repeatedly while running.
- [ ] Reset repeatedly while paused.
- [ ] Change particle count repeatedly.
- [ ] Confirm that ImGui still renders normally after every graphics-state path.
- [ ] Confirm that depth mask is `GL_TRUE` after the particle pass.
- [ ] Confirm that blending is disabled or deliberately configured before ImGui.
- [ ] Confirm that no OpenGL owner outlives its context.

### Run development-quality checks

- [ ] Run `clang-format --dry-run --Werror` over first-party C++.
- [ ] Run clang-tidy through the project option.
- [ ] Build with warnings as errors.
- [ ] Run Catch2/CTest.
- [ ] Run the MSVC AddressSanitizer preset where available.
- [ ] Run the Linux Clang AddressSanitizer and UndefinedBehaviorSanitizer preset.
- [ ] Run Debug smoke testing.
- [ ] Run Release smoke testing.
- [ ] Confirm that interactive mode does not call `glFinish`.

### Understanding check

- [ ] Explain how SSBO, dispatch, and draw limits independently constrain particle count.
- [ ] Explain why implementation limits should be cached rather than queried every frame.
- [ ] Explain why a smoke-only GPU readback is acceptable but an interactive readback is not.
- [ ] Explain which operations may stall the CPU waiting for the GPU.
- [ ] Explain the render states that must be restored after the particle pass.

### Acceptance check

```text
1. The UI exposes only counts that the current implementation can support.
2. Non-multiple work-group counts remain safe.
3. Repeated reset and recreation do not leak or corrupt GPU state.
4. Smoke testing exercises compute, opaque rendering, particle rendering, and synchronization.
5. Debug callbacks, tests, format, tidy, and sanitizers pass.
```

---

## Feature 10: Finish the showcase and document what was learned

**User-visible result:** the repository presents a polished, reproducible GPU particle-system
showcase rather than a renamed lesson bootstrap.

**Suggested commit:** `Polish and document the GPU particle singularity`

### Tune the default presentation

- [ ] Start with `65,536` particles on ordinary supported hardware.
- [ ] Choose a camera that clearly shows the disk thickness and central core.
- [ ] Choose a dark background distinct from the core body.
- [ ] Tune the particle palette for visible age progression.
- [ ] Tune billboard size so individual particles are visible without hiding all structure.
- [ ] Tune attraction, orbital speed, swirl, and drag as a group.
- [ ] Confirm that particles approach the core from varied trajectories.
- [ ] Confirm that the emitter boundary is not distractingly obvious.
- [ ] Confirm that the effect remains readable at `1280 × 720`.
- [ ] Confirm that it remains usable at high-DPI framebuffer sizes.
- [ ] Keep point-debug mode available but disabled by default.
- [ ] Remove temporary debug text and controls that no longer help.

### Replace the starter README

The README should explain:

- [ ] what the application displays;
- [ ] that it is an artistic particle simulation, not black-hole physics;
- [ ] the supported platforms;
- [ ] build, run, test, and sanitizer commands;
- [ ] the default particle count;
- [ ] the particle-state layout;
- [ ] CPU initialization;
- [ ] compute dispatch and fixed-step update;
- [ ] the memory barrier;
- [ ] instanced billboard rendering;
- [ ] additive blending and depth behavior;
- [ ] ImGui controls;
- [ ] project limitations and future work.

Include this data-flow diagram or an equivalent one:

```text
CPU / C++
│
├── settings and seed
├── deterministic initial particles
├── ParticleGpu[] ──────────────────────────────┐
├── fixed-step accumulator                      │
└── ImGui controls                              │
                                                ↓
                                   particle SSBO, binding 0
                                                │
                          ┌─────────────────────┴─────────────────────┐
                          ↓                                           ↓
                  Compute shader                              Billboard vertex shader
                  ├── one invocation per particle             ├── one instance per particle
                  ├── forces                                  ├── reads updated SSBO state
                  ├── integration                             ├── expands six quad vertices
                  ├── lifetime                                └── projects view-space billboard
                  └── respawn                                          │
                          │                                             ↓
                          └── glMemoryBarrier ───────────────→ Rasterizer
                                                                        │
                                                                        ↓
                                                              Fragment shader
                                                              ├── radial soft mask
                                                              ├── lifetime color
                                                              └── source alpha
                                                                        │
                                                                        ↓
                                                       additive framebuffer blending
```

- [ ] Make clear that the same SSBO is written by compute and read by graphics.
- [ ] Explain why a barrier separates those operations.
- [ ] Explain why instancing renders one quad per particle.
- [ ] Explain why additive particles do not require sorting in this version.
- [ ] Keep explanations connected to actual source files and symbols.
- [ ] Remove all remaining starter-specific README instructions that no longer apply.

### Add visual media

- [ ] Capture one clean screenshot.
- [ ] Capture a short GIF or video showing particle motion and the control panel.
- [ ] Avoid enormous uncompressed media in Git history.
- [ ] Use a deterministic seed for reproducible presentation media.
- [ ] Add meaningful alt text.
- [ ] Confirm that README media renders on GitHub.

### Perform final repository review

- [ ] Search for `TriangleDemo` and remove every obsolete reference.
- [ ] Search for old bootstrap identity forms again.
- [ ] Search for `TODO`, `FIXME`, and `HACK` comments and resolve or justify them.
- [ ] Confirm that every first-party file is listed in CMake.
- [ ] Confirm that generated and vendored files are excluded from formatting rules.
- [ ] Confirm that no runtime file path depends on the developer's machine.
- [ ] Confirm that no shader source is duplicated accidentally.
- [ ] Confirm that every OpenGL allocation has one clear owner.
- [ ] Confirm that every owner has correct destruction ordering.
- [ ] Confirm that default settings satisfy all invariants.
- [ ] Confirm that README commands match actual preset and executable names.

### Final validation matrix

- [ ] Windows MSVC Debug configures and builds from a clean tree.
- [ ] Windows MSVC Release configures and builds from a clean tree.
- [ ] Windows tests pass.
- [ ] Windows smoke test passes.
- [ ] Windows interactive run shows no OpenGL debug errors.
- [ ] Ubuntu GCC Debug configures and builds from a clean tree.
- [ ] Ubuntu GCC Release configures and builds from a clean tree.
- [ ] Ubuntu Clang sanitizer build passes.
- [ ] Ubuntu tests pass.
- [ ] Ubuntu smoke test passes under X11 or XWayland.
- [ ] GitHub Actions passes.
- [ ] The final commit contains only intended files.

### Understanding check

- [ ] Explain the complete path from CPU initialization to a blended framebuffer pixel.
- [ ] Explain which particle work remains on the CPU and which happens on the GPU.
- [ ] Explain why this design scales better than uploading every updated particle each frame.
- [ ] Explain the most important limitation that would appear when adding particle-to-particle forces.
- [ ] Explain which parts could transfer conceptually to Vulkan, Direct3D, WebGPU, or a game engine.

### Acceptance check

```text
1. A fresh clone can be built from the README.
2. The default run immediately shows the intended singularity effect.
3. Tests, smoke checks, sanitizers, formatting, and CI pass.
4. The README explains the real pipeline rather than merely advertising the result.
5. The project has a screenshot or animation and a clean final commit.
```

---

# Recommended Final File Structure

This is a guide, not a requirement to create every file before it has content:

```text
src/
  main.cpp

  bootstrap/
    glfw_context.cpp
    glfw_context.hpp

  demo/
    singularity_scene.cpp       # frame orchestration and ImGui
    singularity_scene.hpp
    particle_system.cpp         # SSBO, compute update, point/billboard draw
    particle_system.hpp
    singularity_core.cpp        # opaque sphere rendering
    singularity_core.hpp

  graphics/
    shader_program.cpp          # arbitrary shader-stage lists
    shader_program.hpp
    icosphere.cpp
    icosphere.hpp
    mesh_data.hpp

  simulation/
    particle_data.hpp           # ParticleGpu and layout assertions
    particle_initializer.cpp    # deterministic CPU initialization
    particle_initializer.hpp
    simulation_settings.cpp     # defaults and validation, only if needed
    simulation_settings.hpp

  support/
    app_options.hpp
    opengl_diagnostics.cpp
    opengl_diagnostics.hpp
    smoke_test.cpp
    smoke_test.hpp

  ui/
    imgui_session.cpp
    imgui_session.hpp

tests/
  app_options_test.cpp
  dispatch_math_test.cpp
  icosphere_test.cpp
  particle_initializer_test.cpp
  particle_layout_test.cpp
  simulation_settings_test.cpp
```

Keep orchestration direct. The application does not need `Engine`, `World`, `Entity`, `RenderGraph`,
`ParticleManagerFactory`, or other ceremonial titles. Add a file or class when it owns a real
responsibility, resource, or testable calculation.

---

# Final Frame Order

```text
poll window events
begin ImGui frame
edit settings

measure frame delta
update fixed-step accumulator

for each required fixed substep:
    bind compute program
    bind particle SSBO
    upload simulation uniforms
    dispatch compute work groups
    memory barrier for SSBO access

query framebuffer size
if framebuffer size is nonzero:
    set viewport
    clear color and depth

    render opaque singularity core
        depth test:   enabled
        depth writes: enabled
        blending:     disabled
        culling:      enabled

    render particle billboards
        depth test:   enabled
        depth writes: disabled
        blending:     additive
        culling:      disabled

    restore depth writes and blending state

render ImGui
run smoke-frame validation when requested
swap buffers
```

- [ ] Confirm that simulation does not depend on framebuffer size.
- [ ] Confirm that a minimized framebuffer skips projection and draw work safely.
- [ ] Confirm that compute can remain paused while rendering the current state.
- [ ] Confirm that opaque depth exists before billboards are blended.
- [ ] Confirm that ImGui receives a sane render state.

---

# Mathematics Cheat Sheet

## Uniform random number in a range

For $u\in[0,1)$ and range $[a,b)$:

$$
x = a + u(b-a)
$$

Equivalent GLSL:

```glsl
float x = mix(a, b, u);
```

---

## Uniform annulus sampling by area

Uniform radius sampling:

$$
r = R_{\min} + u(R_{\max}-R_{\min})
$$

puts too many samples per unit area near the inner edge because annulus circumference grows with
radius.

Uniform area sampling uses:

$$
r =
\sqrt{
R_{\min}^{2}
+
u
\left(
R_{\max}^{2}-R_{\min}^{2}
\right)
}
$$

with $\nu\in[0,1)$.

---

## Vector length and normalization

$$
\lVert\mathbf v\rVert
=
\sqrt{\mathbf v\cdot\mathbf v}
$$

$$
\hat{\mathbf v}
=
\frac{\mathbf v}{\lVert\mathbf v\rVert}
$$

Never normalize a vector until its squared length has been checked against a small positive
threshold.

---

## Cross product for a tangent

For world-up vector $\hat{\mathbf y}$ and horizontal radius $\mathbf r_{\perp}$:

$$
\mathbf t_{raw}
=
\hat{\mathbf y}
\times
\mathbf r_{\perp}
$$

$$
\mathbf t
=
\frac{\mathbf t_{raw}}
{\lVert\mathbf t_{raw}\rVert}
$$

Changing operand order reverses direction:

$$
\mathbf a\times\mathbf b
=
-
\left(
\mathbf b\times\mathbf a
\right)
$$

---

## Softened inverse-square attraction

$$
\mathbf a
=
-G
\frac{\mathbf p}
{\left(\mathbf p\cdot\mathbf p+\varepsilon^2\right)^{3/2}}
$$

At large radius, this approaches inverse-square magnitude. Near zero, softening prevents an
unbounded acceleration.

---

## Linear drag

$$
\mathbf a_{drag}=-D\mathbf v
$$

This is a visual and numerical model, not a complete fluid-dynamics treatment.

---

## Semi-implicit Euler

$$
\mathbf v_{n+1}
=
\mathbf v_n+
\mathbf a_n\Delta t
$$

$$
\mathbf p_{n+1}
=
\mathbf p_n+
\mathbf v_{n+1}\Delta t
$$

The integration remains approximate. Smaller fixed steps generally reduce error at the cost of
more compute dispatches.

---

## Work-group count

$$
G=
\left\lceil
\frac{N}{L}
\right\rceil
$$

Where:

- $N$ is active particle count;
- $L$ is local work-group size;
- $G$ is dispatched work-group count.

Every invocation must still verify:

```glsl
if (gl_GlobalInvocationID.x >= uParticleCount) {
    return;
}
```

---

## Lifetime progress

$$
q=
\operatorname{clamp}
\left(
\frac{a}{T},0,1
\right)
$$

Use $q$ to derive appearance. Do not store values that can be cheaply reconstructed from age and
lifetime.

---

## View-space billboard expansion

$$
\mathbf c_v=V\mathbf c_w
$$

$$
\mathbf p_v=
\mathbf c_v+
\begin{bmatrix}
sq_x \\
sq_y \\
0 \\
0
\end{bmatrix}
$$

$$
\mathbf p_{clip}=P\mathbf p_v
$$

All corners share the center's view-space depth, while X and Y offsets make the quad face the
camera.

---

## Radial billboard mask

$$
\rho=2\left\lVert\mathbf{uv}-(1/2,1/2)\right\rVert
$$

$$
m=1-\operatorname{smoothstep}(r_{soft},1,\rho)
$$

Fragments with $\rho>1$ are outside the disc.

---

## Additive blending

With:

```cpp
glBlendFunc(GL_SRC_ALPHA, GL_ONE);
```

The conceptual color update is:

$$
C_{out}
=
\alpha_s C_s + C_d
$$

Where:

- $C_s$ is source particle color;
- $\alpha_s$ is source alpha;
- $C_d$ is current destination color.

Because contributions are added, exact back-to-front particle order is not required for this
stylized effect.

---

# Troubleshooting

## The static particle cloud is invisible

Check:

- [ ] The SSBO contains a nonzero particle count.
- [ ] The buffer is bound to the shader's declared binding index.
- [ ] C++ and GLSL structures both use the expected `48`-byte stride.
- [ ] An empty VAO is bound before the core-profile draw.
- [ ] `GL_PROGRAM_POINT_SIZE` is enabled for diagnostic points.
- [ ] `gl_PointSize` is positive and within the implementation's supported range.
- [ ] View and projection matrices place the annulus inside the frustum.
- [ ] The framebuffer dimensions are nonzero.
- [ ] Shader compilation and program linking logs are empty.
- [ ] The OpenGL debug callback reports no error.

---

## Only some particles move

Check:

- [ ] Work-group count uses ceiling division.
- [ ] `uParticleCount` matches the active draw count.
- [ ] The compute shader uses `gl_GlobalInvocationID.x`.
- [ ] The bounds check is `index >= particleCount`, not the reverse.
- [ ] The SSBO is bound before dispatch.
- [ ] The compute program is active before uniforms and dispatch.

---

## Particle motion appears one frame late or flickers

Check:

- [ ] `glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT)` occurs after dispatch.
- [ ] The barrier occurs after every fixed substep.
- [ ] Rendering reads the same buffer that compute writes.
- [ ] No stale buffer object remains bound after particle-count recreation.
- [ ] C++ and GLSL agree on field layout.

---

## Particles explode near the origin

Check:

- [ ] Softening is strictly positive.
- [ ] Core capture radius is positive.
- [ ] Frame delta is bounded.
- [ ] Fixed-step substeps are active.
- [ ] Acceleration sign points toward the origin.
- [ ] Semi-implicit integration updates velocity before position.
- [ ] No zero-length vector is normalized.
- [ ] Optional acceleration clamping uses a sensible maximum.
- [ ] Non-finite states trigger respawn.

---

## Every particle follows nearly the same path

Check:

- [ ] Every particle has a distinct seed derived from index and global seed.
- [ ] No random state is zero.
- [ ] Random state is written back after respawn.
- [ ] Velocity jitter consumes fresh random values.
- [ ] Initial ages are staggered.
- [ ] Emitter thickness is nonzero when a three-dimensional cloud is desired.

---

## Billboards appear as squares

Check:

- [ ] UV coordinates cover `[0,1]` across every quad.
- [ ] Radial distance is measured from `(0.5, 0.5)`.
- [ ] Fragments outside the unit circle are discarded.
- [ ] Alpha includes the radial soft mask.
- [ ] Additive blending is enabled during the particle pass.

---

## Particles are visible through the central sphere

Check:

- [ ] The depth buffer is cleared each frame.
- [ ] The core renders before particles.
- [ ] Depth testing is enabled for both passes.
- [ ] Core depth writes are enabled.
- [ ] Particle depth writes are disabled only after the core pass.
- [ ] Core and particle shaders use compatible view and projection matrices.

---

## ImGui becomes transparent, invisible, or otherwise corrupted

Check:

- [ ] `glDepthMask(GL_TRUE)` is restored after particles.
- [ ] Particle blending is disabled before ImGui.
- [ ] Polygon mode is `GL_FILL`.
- [ ] The intended VAO/program state is not assumed across ImGui rendering.
- [ ] ImGui begins and renders exactly once per frame.

---

## Performance is unexpectedly poor

Check:

- [ ] The application is not running an unoptimized Debug build for performance measurement.
- [ ] Interactive mode does not call `glFinish` or read particle data back.
- [ ] Particle count is what the UI claims it is.
- [ ] The fixed-step accumulator is not dispatching the maximum substep count every frame.
- [ ] The billboard fragment shader discards outside-circle fragments early enough to help.
- [ ] Billboard size is not causing massive full-screen overdraw.
- [ ] The application is not reallocating the particle buffer every frame.
- [ ] OpenGL debug output is not flooding the console.

Do not respond to a performance problem by immediately building a render graph, indirect-command
system, and custom allocator. Measure the actual compute, draw, synchronization, and overdraw costs
first.

---

# Final Stopping Criterion

The first project version is finished when:

1. particle state is initialized on the CPU and then simulated persistently in a compute shader;
2. attraction, vortex force, drag, lifetime, core capture, escape, and respawn all work;
3. fixed stepping prevents frame-time spikes from destabilizing the system;
4. instanced soft billboards render with additive blending;
5. the opaque core correctly occludes particles;
6. ImGui provides useful controls and diagnostics;
7. count limits, reset, resize, pause, and smoke-test paths behave reliably;
8. tests, format, static analysis, sanitizers, Debug, Release, and CI pass;
9. the README explains the actual `CPU → SSBO → compute → barrier → graphics → blend` pipeline;
10. the repository contains presentation media and one final coherent polish commit.

Then:

- [ ] Make the final feature commit.
- [ ] Record remaining ideas under `Future work`.
- [ ] Close every completed checkbox that genuinely matches the implementation.
- [ ] Leave deliberately skipped tasks unchecked with a brief explanation.
- [ ] Stop expanding the first version.

A sudden desire for bloom, gravitational lensing, particle collisions, trails, an ECS, or a Vulkan
port is evidence that `Future work` needs another bullet, not evidence that the project is unfinished.
