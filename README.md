# gpu-particle-singularity

`gpu-particle-singularity` is a C++23/OpenGL 4.6 project for a GPU-driven particle
singularity simulation. It builds one executable with strict warnings, lightweight RAII wrappers,
Dear ImGui, Catch2 tests, sanitizer presets, and a real OpenGL smoke test. The interactive
triangle is temporary proof that the branded project baseline works before particle rendering is
introduced.

## Platform scope

- Windows x64 is supported with MSVC and Ninja.
- Ubuntu 24.04 x64 is supported with GCC or Clang. GLFW uses X11; Wayland sessions require
  XWayland and a valid `DISPLAY`.
- macOS is unsupported because Apple does not provide OpenGL 4.6 or GLSL 460. CI verifies the
  explicit configure-time rejection.

## Dependencies

The build requires CMake 3.25 or newer, Ninja, a C++23 compiler, Git, and vcpkg in manifest mode.
The manifest pins vcpkg commit `9e593bb18ea69cc5095e012465dcd675a822ed0d`, published as tag
`2026.07.29`, and installs GLFW, Dear ImGui, GLM, and Catch2.

GLAD2 is vendored because vcpkg's `glad` port provides GLAD1. The checked-in OpenGL 4.6 Core
loader was reproducibly generated with GLAD2 v2.0.8 at commit
`73db193f853e2ee079bf3ca8a64aa2eaf6459043`; see `third_party/glad/PROVENANCE.md` for the exact
command. Building the project does not require Python.

Clone and bootstrap the pinned vcpkg checkout from the repository root.

Windows PowerShell:

```powershell
git clone --depth 1 --branch 2026.07.29 https://github.com/microsoft/vcpkg.git .tools/vcpkg
git -C .tools/vcpkg rev-parse HEAD
.tools\vcpkg\bootstrap-vcpkg.bat -disableMetrics
```

Linux:

```bash
git clone --depth 1 --branch 2026.07.29 https://github.com/microsoft/vcpkg.git .tools/vcpkg
git -C .tools/vcpkg rev-parse HEAD
.tools/vcpkg/bootstrap-vcpkg.sh -disableMetrics
```

In both cases, `rev-parse` must print the pinned commit above. The presets use the checkout at
`.tools/vcpkg` directly, so `VCPKG_ROOT` is not required. Ubuntu also needs GLFW's X11 development
packages:

```bash
sudo apt-get update
sudo apt-get install --yes build-essential clang clang-format clang-tidy cmake gdb ninja-build \
  pkg-config libglu1-mesa-dev libxcursor-dev libxinerama-dev xorg-dev
```

The ignored `.tools/` and `out/` directories hold checkout-local tools, dependencies, and build
artifacts.

## Configure and build

Run Windows commands from an x64 Visual Studio developer shell:

```powershell
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug

cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release
```

If CMake or Ninja is not installed globally, the local vcpkg can fetch portable copies:

```powershell
$cmake = (& .\.tools\vcpkg\vcpkg.exe fetch cmake | Select-Object -Last 1)
$ninja = (& .\.tools\vcpkg\vcpkg.exe fetch ninja | Select-Object -Last 1)
$env:Path = "$(Split-Path $ninja);$env:Path"
& $cmake --preset windows-msvc-debug
& $cmake --build --preset windows-msvc-debug
```

Linux GCC presets:

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug

cmake --preset linux-gcc-release
cmake --build --preset linux-gcc-release
```

Each preset uses an isolated `out/build/<preset>` tree and exports
`compile_commands.json`. vcpkg install trees are separated by host platform so Windows and Linux
builds can share the same checkout.

## VS Code workflow

After bootstrapping the pinned `.tools/vcpkg` checkout above, open the repository in an ordinary
VS Code or VS Code Insiders window and install the recommended CMake Tools, clangd, and C/C++
extensions. The tracked workspace contains no machine-specific tool paths. On Windows, CMake Tools
discovers Visual Studio's bundled CMake and Ninja, enters the x64 MSVC developer environment, and
uses the `windows-msvc-debug` preset; a preconfigured developer shell is not required. On Linux,
the same workflow uses the `linux-gcc-debug` preset and the CMake, Ninja, GCC, and GDB installations
available on `PATH`.

Run `CMake: Configure Debug` and `CMake: Build Debug` from **Terminal > Run Task**; the latter is
also the default build task. After configuration, the workspace setting copies the selected build
tree's compilation database to `out/compile_commands.json`. clangd reads that stable path for
indexing, formatting, and C++23 header inference. Use clangd 22 or newer; earlier versions can
mis-infer the language mode for C++23 headers. The clangd extension can install and manage that
language server independently of the system Clang packages. The C/C++ extension remains installed
for its Windows and Linux debuggers, while its IntelliSense engine stays disabled.

The Run and Debug view provides `Debug: Interactive (Windows)`, `Debug: Smoke test (Windows)`,
`Debug: Interactive (Linux)`, and `Debug: Smoke test (Linux)`. They build first and use the
executable path resolved by CMake Tools; the smoke variants pass `--smoke-test` and exit after three
frames. No configuration depends on an absolute path or a fixed output filename.

## Run, smoke test, and test

Start the interactive application:

```powershell
.\out\build\windows-msvc-debug\gpu-particle-singularity.exe
```

```bash
./out/build/linux-gcc-debug/gpu-particle-singularity
```

The application accepts `--help` (or `-h`) and `--smoke-test`. The smoke test creates a real OpenGL
context, renders and validates three frames, then exits automatically:

```powershell
.\out\build\windows-msvc-debug\gpu-particle-singularity.exe --smoke-test
ctest --preset windows-msvc-debug
```

```bash
./out/build/linux-gcc-debug/gpu-particle-singularity --smoke-test
ctest --preset linux-gcc-debug
```

Invalid arguments exit with status `2`; initialization or rendering failures exit with status
`1`. Catch2 cases are discovered individually by CTest.

## Sanitizers

MSVC AddressSanitizer:

```powershell
cmake --preset windows-msvc-asan
cmake --build --preset windows-msvc-asan
ctest --preset windows-msvc-asan
```

This preset requires Visual Studio's optional MSVC AddressSanitizer runtime. Configuration fails
with a direct diagnostic when the runtime is unavailable. First-party targets keep STL
annotations ABI-compatible with the non-instrumented vcpkg libraries while retaining
AddressSanitizer instrumentation.

Linux Clang AddressSanitizer plus UndefinedBehaviorSanitizer:

```bash
cmake --preset linux-clang-asan
cmake --build --preset linux-clang-asan
ctest --preset linux-clang-asan
```

Sanitizer flags apply only to first-party targets.

## Formatting and static analysis

Check handwritten C++ formatting on Windows:

```powershell
$sourceFiles = Get-ChildItem src,tests -Recurse -File -Include *.cpp,*.hpp
clang-format --dry-run --Werror $sourceFiles.FullName
```

Linux:

```bash
find src tests -type f \( -name '*.cpp' -o -name '*.hpp' \) -print0 \
  | xargs -0 clang-format --dry-run --Werror
```

Replace `--dry-run --Werror` with `-i` to apply formatting. Generated GLAD2 sources are excluded.

Enable clang-tidy for compiled first-party translation units:

```powershell
cmake --preset windows-msvc-debug -DGPS_ENABLE_CLANG_TIDY=ON
cmake --build --preset windows-msvc-debug
```

Use `linux-gcc-debug` for the equivalent Linux build. Configuration fails when clang-tidy is
requested but unavailable. CI additionally configures first-party targets with
`GPS_WARNINGS_AS_ERRORS=ON`.

## Reuse as a project template

Use the repository as a template or copy its working tree without `.git` when another project
needs independent history. Replace these four identity forms consistently across first-party
files:

- `gpu-particle-singularity`: repository, executable, package, and user-facing kebab-case name;
- `gpu_particle_singularity`: CMake project and target names;
- `gps`: C++ namespace and CMake function prefix;
- `GPS`: CMake option and internal variable prefix.

Also update the CMake project description and window title, remove or replace the sample triangle,
and run a repository-wide search to confirm the old identity is gone:

```powershell
rg -n --glob '!third_party/glad/**' 'gpu-particle-singularity|gpu_particle_singularity|\bgps\b|GPS' .
```

Keep `third_party/glad` unchanged unless the required OpenGL version or extension set changes.
