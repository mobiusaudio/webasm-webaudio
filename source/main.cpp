#include <SDL2/SDL.h>
#include <GLES3/gl3.h>

#include <emscripten.h>
#include <emscripten/html5.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include "AudioEngine.h"

SDL_Window* g_Window = nullptr;
SDL_GLContext g_GLContext = nullptr;
Parameters g_Params;
AudioEngine g_AudioEngine(g_Params);

void MainLoop()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("C++ DSP Rig");

    if (!g_Params.audioRunning.load())
    {
        if (ImGui::Button("Start Audio Engine", ImVec2(50.0f, 100.0f)))
        {
            g_AudioEngine.Start();
        }
    }
    else
    {
        ImGui::TextColored(ImVec4(0,1,0,1), "Audio Engine Running");
        ImGui::Spacing();

        float gain = g_Params.masterGain.load();
        if (ImGui::SliderFloat("Master Gain", &gain, 0.0f, 2.0f))
        {
            g_Params.masterGain.store(gain, std::memory_order_relaxed);
        }
    }

    ImGui::End();

    ImGui::Render();
    glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(g_Window);
}

EM_BOOL OnWindowResized(int eventType, const EmscriptenUiEvent *uiEvent, void *userData)
{
    double width, height;
    emscripten_get_element_css_size("#canvas", &width, &height);
    SDL_SetWindowSize(g_Window, (int)width, (int)height);
    return EM_TRUE;
}

int main()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Ask the browser for the initial canvas size on load
    double width, height;
    emscripten_get_element_css_size("#canvas", &width, &height);

    // Create the window using the dynamic browser dimensions
    g_Window = SDL_CreateWindow("ImGui WebAudio", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        (int)width, (int)height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
    g_GLContext = SDL_GL_CreateContext(g_Window);

    // Tell Emscripten to fire our callback when the browser resizes
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, false, OnWindowResized);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(g_Window, g_GLContext);
    ImGui_ImplOpenGL3_Init("#version 300 es");

    emscripten_set_main_loop(MainLoop, 0, true);
    return 0;
}
