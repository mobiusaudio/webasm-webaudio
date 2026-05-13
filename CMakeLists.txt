cmake_minimum_required(VERSION 3.16)
project(WasmAudioGain CXX)

set(CMAKE_CXX_STANDARD 17)

# --- FetchContent Setup ---
include(FetchContent)

# Placeholder: This is exactly where you use FetchContent to grab external DSP libraries.
# Example:
# FetchContent_Declare(
#     MyDSPLib
#     GIT_REPOSITORY https://github.com/Example/MyDSPLib.git
#     GIT_TAG        main
# )
# FetchContent_MakeAvailable(MyDSPLib)

# --- Executable Setup ---
add_executable(audio_app main.cpp)

# --- Emscripten Linker Flags ---
# These flags are critical. They enable WebAudio Worklets, Wasm Workers, 
# and export our specific C++ functions so JS can trigger them.
target_link_options(audio_app PRIVATE
    "-sAUDIO_WORKLET=1"
    "-sWASM_WORKERS=1"
    "-sEXPORTED_FUNCTIONS=['_main','_InitAudio','_SetGain']"
    "-sEXPORTED_RUNTIME_METHODS=['ccall']"
    "--no-entry" # We handle initialization explicitly
)