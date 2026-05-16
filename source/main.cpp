#include <SDL2/SDL.h>
#include <GLES3/gl3.h>

#include <emscripten.h>
#include <emscripten/html5.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include "AudioEngine.h"

#include <cmath>

SDL_Window* g_Window = nullptr;
SDL_GLContext g_GLContext = nullptr;
Parameters g_Params;
AudioEngine g_AudioEngine(g_Params);

bool DrawCustomKnob(const char* label, float* value, float v_min, float v_max)
{
    bool value_changed = false;

    float radius = 25.0f;
    ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
    ImVec2 center(cursor_pos.x + radius, cursor_pos.y + radius);

    ImGui::InvisibleButton(label, ImVec2(radius * 2.0f, radius * 2.0f + 20.0f));

    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        float drag_delta = ImGui::GetIO().MouseDelta.y;
        float step = (v_max - v_min) * 0.01f;
        *value -= drag_delta * step;

        if (*value < v_min) *value = v_min;
        if (*value > v_max) *value = v_max;
        value_changed = true;
    }

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImU32 bg_color = ImGui::IsItemHovered() ? IM_COL32(70, 70, 70, 255) : IM_COL32(50, 50, 50, 255);
    ImU32 indicator_color = IM_COL32(255, 165, 0, 255); // Orange

    draw_list->AddCircleFilled(center, radius, bg_color, 32);
    draw_list->AddCircle(center, radius, IM_COL32(30, 30, 30, 255), 32, 2.0f); // Dark border

    float t = (*value - v_min) / (v_max - v_min); // Normalized 0.0 to 1.0
    float angle = -135.0f + (t * 270.0f);         // Map to an arc (-135 to +135 degrees)
    float angle_rad = angle * (3.14159f / 180.0f);
    ImVec2 line_end(center.x + sinf(angle_rad) * (radius - 4.0f), center.y - cosf(angle_rad) * (radius - 4.0f));
    draw_list->AddLine(center, line_end, indicator_color, 3.0f);

    ImVec2 text_size = ImGui::CalcTextSize(label);
    ImVec2 text_pos(center.x - (text_size.x * 0.5f), center.y + radius + 4.0f);
    draw_list->AddText(text_pos, IM_COL32(200, 200, 200, 255), label);

    return value_changed;
}

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
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin("C++ DSP Rig");

    if (!g_Params.audioRunning.load())
    {
        if (ImGui::Button("Start Audio Engine", ImVec2(-1.0f, 50.0f)))
        {
            g_AudioEngine.Start();
        }
    }
    else
    {
        ImGui::TextColored(ImVec4(0,1,0,1), "Audio Engine Running");
        ImGui::Spacing();

        float gain = g_Params.masterGain.load();
        if (DrawCustomKnob("Master", &gain, 0.0f, 2.0f))
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
