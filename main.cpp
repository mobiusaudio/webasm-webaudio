#include <emscripten.h>
#include <emscripten/webaudio.h>
#include <cstdio> // <-- Changed from <iostream>

// Dedicated stack for the WebAudio thread
uint8_t audioThreadStack[4096];

// Shared state
float g_gain = 0.5f;

// -----------------------------------------------------------------
// 1. The DSP Loop (Runs directly on the browser's AudioWorklet Thread)
// -----------------------------------------------------------------
bool ProcessAudio(int numInputs, const AudioSampleFrame *inputs,
                  int numOutputs, AudioSampleFrame *outputs,
                  int numParams, const AudioParamFrame *params,
                  void *userData) {
                  
    for (int i = 0; i < numOutputs; i++) {
        int channels = outputs[i].numberOfChannels;
        int samples = outputs[i].samplesPerChannel;

        for (int ch = 0; ch < channels; ch++) {
            float* outData = outputs[i].data + (ch * samples);
            const float* inData = nullptr;

            if (i < numInputs && ch < inputs[i].numberOfChannels) {
                inData = inputs[i].data + (ch * inputs[i].samplesPerChannel);
            }

            for (int s = 0; s < samples; s++) {
                outData[s] = inData ? (inData[s] * g_gain) : 0.0f;
            }
        }
    }
    return true; 
}

// -----------------------------------------------------------------
// 2. Node Initialization Callbacks
// -----------------------------------------------------------------
void OnWorkletProcessorCreated(EMSCRIPTEN_WEBAUDIO_T audioContext, bool success, void *userData) {
    if (!success) {
        printf("Failed to create Wasm AudioWorklet Processor!\n"); // <-- Changed to printf
        return;
    }

    EmscriptenAudioWorkletNodeCreateOptions options = {0};
    options.numberOfInputs = 1;
    options.numberOfOutputs = 1;
    
    int outputChannelCounts[1] = {2}; 
    options.outputChannelCounts = outputChannelCounts;

    EMSCRIPTEN_AUDIO_WORKLET_NODE_T workletNode = emscripten_create_wasm_audio_worklet_node(
        audioContext, "GainProcessor", &options, &ProcessAudio, nullptr);

    emscripten_audio_node_connect(workletNode, audioContext, 0, 0);

    EM_ASM({
        window.wasmWorkletNode = emscriptenGetAudioObject($0);
        window.dispatchEvent(new Event('WorkletReady')); 
    }, workletNode);
}

void OnThreadInitialized(EMSCRIPTEN_WEBAUDIO_T audioContext, bool success, void *userData) {
    if (!success) return;
    WebAudioWorkletProcessorCreateOptions opts = {};
    opts.name = "GainProcessor";
    emscripten_create_wasm_audio_worklet_processor_async(audioContext, &opts, &OnWorkletProcessorCreated, nullptr);
}

// -----------------------------------------------------------------
// 3. Exposed C++ API (Callable from JavaScript)
// -----------------------------------------------------------------
extern "C" {
    EMSCRIPTEN_KEEPALIVE void SetGain(float val) {
        g_gain = val;
    }

    EMSCRIPTEN_KEEPALIVE void InitAudio() {
        EMSCRIPTEN_WEBAUDIO_T context = emscripten_create_audio_context(nullptr);
        
        EM_ASM({ window.audioContext = emscriptenGetAudioObject($0); }, context);

        emscripten_start_wasm_audio_worklet_thread_async(
            context, audioThreadStack, sizeof(audioThreadStack), &OnThreadInitialized, nullptr);
    }
}

int main() {
    printf("Wasm module loaded. Waiting for user to start audio...\n"); // <-- Changed to printf
    return 0;
}