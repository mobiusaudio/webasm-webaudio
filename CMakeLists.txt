cmake_minimum_required(VERSION 3.16)
project(WasmAudioGain CXX)

set(CMAKE_CXX_STANDARD 17)

# --- FetchContent Setup (Dear ImGui) ---
include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.4 # Pinning to a stable release
)
FetchContent_MakeAvailable(imgui)

# Define ImGui source files
set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# --- Executable Setup ---
add_executable(audio_app source/main.cpp source/AudioEngine.cpp ${IMGUI_SOURCES})

target_compile_options(audio_app PRIVATE
    "-sWASM_WORKERS=1"
    "-sUSE_SDL=2"
    "-pthread"
    "-Wno-nontrivial-memcall"
)

# Tell CMake where to find ImGui headers
target_include_directories(audio_app PRIVATE ${IMGUI_DIR} ${IMGUI_DIR}/backends)

# --- Emscripten Linker Flags ---
target_link_options(audio_app PRIVATE
    "-pthread"
    "-sPTHREAD_POOL_SIZE=4"
    "-sAUDIO_WORKLET=1"
    "-sWASM_WORKERS=1"
    "-sUSE_SDL=2"                 # Enable Emscripten's built-in SDL2
    "-sMAX_WEBGL_VERSION=2"       # Enable WebGL 2 (OpenGL ES 3)
    "-sMIN_WEBGL_VERSION=2"
    "-sEXPORTED_FUNCTIONS=['_main']"
    "-sEXPORTED_RUNTIME_METHODS=['ccall']"

    # Debug vs Release flags
    "$<IF:$<CONFIG:Debug>,-O0,-O3>"
    "$<IF:$<CONFIG:Debug>,-g,-g0>"
    "$<IF:$<CONFIG:Debug>,-gsource-map,>"
)
