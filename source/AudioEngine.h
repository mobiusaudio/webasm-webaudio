#pragma once

#include <emscripten/webaudio.h>
#include "Parameters.h"

class AudioEngine {
public:
    AudioEngine(Parameters& params);
    ~AudioEngine() = default;

    void Start();

private:
    // The actual DSP loop
    static bool ProcessAudio(int numInputs, const AudioSampleFrame *inputs,
                             int numOutputs, AudioSampleFrame *outputs,
                             int numParams, const AudioParamFrame *params,
                             void *userData);

    // Initialization callbacks
    static void OnWorkletProcessorCreated(EMSCRIPTEN_WEBAUDIO_T audioContext, bool success, void *userData);
    static void OnThreadInitialized(EMSCRIPTEN_WEBAUDIO_T audioContext, bool success, void *userData);

    Parameters& m_params;
    alignas(16) uint8_t m_audioThreadStack[4096];
};