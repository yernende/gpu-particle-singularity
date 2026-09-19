# Screenshot provenance

These images were captured on September 20, 2026, from the current production
simulation and renderer on Windows with an NVIDIA GeForce RTX 3060 Ti
(OpenGL 4.6, NVIDIA driver 591.86).

| Image | Capture |
| --- | --- |
| `singularity-default.png` | Scene after 720 fixed simulation steps (6 seconds), before drawing the controls. |
| `particle-laboratory.png` | The same scene with the real Dear ImGui controls, moved to the left and enlarged for readability. |

Both images are 1920 x 1080 with 4x MSAA. They use the unmodified defaults:
4,097 particles, seed 12345, billboard rendering, and the application's fixed
camera, forces, emitter, colors, and clear color.

A temporary harness linked the production `GpsDemo`, GLFW context, graphics,
simulation, and ImGui source files. It created a hidden GLFW window, advanced
`GpsDemo::step_simulation_once()` at the default 1/120-second step, and called
`GpsDemo::draw()` with the same OpenGL state as `main.cpp`. Pixels were read
from the real GPU framebuffer with `glReadPixels` and converted losslessly
from PPM to PNG. No image retouching, compositing, or simulated particle art
was used.

The hero image omits the UI by capturing before the ImGui render pass; the
application does not currently expose a UI-hide shortcut or screenshot command.
The control-panel image comes from the same capture session, so its frame-time
readout is not a real-time performance benchmark. The capture harness and build
outputs are local, ignored files under `out/portfolio-capture`.
