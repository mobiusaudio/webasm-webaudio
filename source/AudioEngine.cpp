#include "AudioEngine.h"

AudioEngine::AudioEngine(Parameters& params) : m_params(params) {}

void AudioEngine::Start()
{
    EmscriptenWebAudioCreateAttributes attrs;
    attrs.latencyHint = "interactive";
    attrs.sampleRate = 48000;
    attrs.renderSizeHint = AUDIO_CONTEXT_RENDER_SIZE_HARDWARE;

    EMSCRIPTEN_WEBAUDIO_T context = emscripten_create_audio_context(&attrs);
    EM_ASM({ window.audioContext = emscriptenGetAudioObject($0); }, context);

    // Pass 'this' as the userData pointer so callbacks can access this instance
    emscripten_start_wasm_audio_worklet_thread_async(context,
        m_audioThreadStack, sizeof(m_audioThreadStack),
        &OnThreadInitialized, this);
}

void AudioEngine::OnThreadInitialized(EMSCRIPTEN_WEBAUDIO_T audioContext, bool success, void *userData)
{
    if (!success) return;
    WebAudioWorkletProcessorCreateOptions opts = {};
    opts.name = "GainProcessor";
    emscripten_create_wasm_audio_worklet_processor_async(audioContext, &opts, &OnWorkletProcessorCreated, userData);
}

void AudioEngine::OnWorkletProcessorCreated(EMSCRIPTEN_WEBAUDIO_T audioContext, bool success, void *userData)
{
    if (!success) return;

    EmscriptenAudioWorkletNodeCreateOptions options = {0};
    options.numberOfInputs = 1;
    options.numberOfOutputs = 1;
    int outputChannelCounts[1] = {2};
    options.outputChannelCounts = outputChannelCounts;

    // Pass userData (our AudioEngine instance) to the processor
    EMSCRIPTEN_AUDIO_WORKLET_NODE_T workletNode = emscripten_create_wasm_audio_worklet_node(audioContext,
        "GainProcessor", &options,
        &ProcessAudio, userData);

    emscripten_audio_node_connect(workletNode, audioContext, 0, 0);

    // Notify JS to connect the microphone
    EM_ASM({
        window.wasmWorkletNode = emscriptenGetAudioObject($0);
        window.ConnectMicrophone();
    }, workletNode);

    auto* engine = static_cast<AudioEngine*>(userData);
    engine->m_params.audioRunning.store(true);
}

bool AudioEngine::ProcessAudio(int numInputs, const AudioSampleFrame *inputs,
                               int numOutputs, AudioSampleFrame *outputs,
                               int numParams, const AudioParamFrame *params,
                               void *userData)
{
    // Cast userData back to our class instance
    auto* engine = static_cast<AudioEngine*>(userData);

    // Lock-free read of the atomic parameter
    float currentGain = engine->m_params.masterGain.load(std::memory_order_relaxed);

    for (int i = 0; i < numOutputs; i++)
    {
        int channels = outputs[i].numberOfChannels;
        int samples = outputs[i].samplesPerChannel;
        for (int ch = 0; ch < channels; ch++)
        {
            float* outData = outputs[i].data + (ch * samples);
            const float* inData = nullptr;

            if (i < numInputs && ch < inputs[i].numberOfChannels)
            {
                inData = inputs[i].data + (ch * inputs[i].samplesPerChannel);
            }

            for (int s = 0; s < samples; s++)
            {
                // Apply DSP
                outData[s] = inData ? (inData[s] * currentGain) : 0.0f;
            }
        }
    }
    return true;
}