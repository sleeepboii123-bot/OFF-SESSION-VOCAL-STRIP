#include "VocalStripDSP.h"
#include <cmath>
#include <iostream>
#include <array>

namespace
{
bool allFinite(const juce::AudioBuffer<float>& buffer)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            if (!std::isfinite(buffer.getReadPointer(ch)[i]))
                return false;
    return true;
}

bool differsFrom(const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b, float tolerance = 1.0e-5f)
{
    if (a.getNumChannels() != b.getNumChannels() || a.getNumSamples() != b.getNumSamples())
        return true;
    for (int ch = 0; ch < a.getNumChannels(); ++ch)
        for (int i = 0; i < a.getNumSamples(); ++i)
            if (std::abs(a.getReadPointer(ch)[i] - b.getReadPointer(ch)[i]) > tolerance)
                return true;
    return false;
}

juce::AudioBuffer<float> makeTestSignal(int samples, double sampleRate)
{
    juce::AudioBuffer<float> signal(2, samples);
    for (int i = 0; i < samples; ++i)
    {
        const double t = static_cast<double>(i) / sampleRate;
        const float value = 0.22f * static_cast<float>(std::sin(2.0 * 3.141592653589793 * 180.0 * t))
                          + 0.17f * static_cast<float>(std::sin(2.0 * 3.141592653589793 * 1000.0 * t))
                          + 0.14f * static_cast<float>(std::sin(2.0 * 3.141592653589793 * 6500.0 * t));
        signal.setSample(0, i, value);
        signal.setSample(1, i, value * 0.93f);
    }
    return signal;
}
}

int main()
{
    using namespace offsession;
    const std::array<int, 8> order { 0, 1, 2, 3, 4, 5, 6, 7 };
    std::array<bool, 8> disabled {};

    for (const double rate : { 44100.0, 48000.0, 96000.0 })
    {
        VocalStripDSP dsp;
        dsp.prepare(rate, 1024, 2);
        juce::AudioBuffer<float> dry(2, 1024);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < dry.getNumSamples(); ++i)
                dry.setSample(ch, i, 0.24f * std::sin(2.0 * 3.141592653589793 * 440.0 * i / rate));

        juce::AudioBuffer<float> reference(2, 1024);
        reference.makeCopyOf(dry, true);
        VocalStripParameters parameters;
        dsp.process(dry, parameters, order, disabled);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < dry.getNumSamples(); ++i)
                if (std::abs(dry.getSample(ch, i) - reference.getSample(ch, i)) > 1.0e-6f)
                {
                    std::cerr << "Dry path changed while all modules were bypassed.\n";
                    return 1;
                }

        dsp.reset();
        std::array<bool, 8> enabled { true, true, true, true, true, true, true, true };
        parameters.optoPeak = 48.0f;
        parameters.optoGainDb = 2.0f;
        parameters.deessAmount = 60.0f;
        parameters.consoleDrive = 15.0f;
        parameters.airMid = 25.0f;
        parameters.airHigh = 30.0f;
        parameters.fetThresholdDb = -20.0f;
        parameters.reverbMix = 0.0f;
        parameters.delayMix = 0.0f;
        dry.makeCopyOf(reference, true);
        dsp.process(dry, parameters, order, enabled);
        if (!allFinite(dry))
        {
            std::cerr << "Non-finite output at sample rate " << rate << ".\n";
            return 2;
        }
    }

    // Reorder handling must continue to process a complete, unique module sequence.
    VocalStripDSP reordered;
    reordered.prepare(48000.0, 512, 2);
    juce::AudioBuffer<float> test(2, 512);
    test.clear();
    for (int i = 0; i < test.getNumSamples(); ++i)
    {
        const float s = 0.15f * std::sin(2.0 * 3.141592653589793 * 180.0 * i / 48000.0);
        test.setSample(0, i, s);
        test.setSample(1, i, s);
    }
    const std::array<int, 8> alternateOrder { 1, 0, 2, 4, 3, 5, 7, 6 };
    std::array<bool, 8> enabled { true, true, true, true, true, true, true, true };
    VocalStripParameters p;
    p.reverbMix = 0.0f;
    p.delayMix = 0.0f;
    reordered.process(test, p, alternateOrder, enabled);
    if (!allFinite(test))
    {
        std::cerr << "Non-finite output with a reordered chain.\n";
        return 3;
    }

    // Reordering a nonlinear compressor around an EQ must change the rendered signal.
    VocalStripDSP normalOrderDsp, swappedOrderDsp;
    normalOrderDsp.prepare(48000.0, 2048, 2);
    swappedOrderDsp.prepare(48000.0, 2048, 2);
    auto normalSignal = makeTestSignal(2048, 48000.0);
    auto swappedSignal = makeTestSignal(2048, 48000.0);
    VocalStripParameters orderSettings;
    orderSettings.optoPeak = 100.0f;
    orderSettings.optoGainDb = 0.0f;
    orderSettings.peqBands[0] = { 180.0f, 12.0f, 1.0f };
    orderSettings.reverbMix = 0.0f;
    orderSettings.delayMix = 0.0f;
    const std::array<int, 8> optoThenEq { 0, 1, 2, 3, 4, 5, 6, 7 };
    const std::array<int, 8> eqThenOpto { 1, 0, 2, 3, 4, 5, 6, 7 };
    normalOrderDsp.process(normalSignal, orderSettings, optoThenEq, enabled);
    swappedOrderDsp.process(swappedSignal, orderSettings, eqThenOpto, enabled);
    if (!differsFrom(normalSignal, swappedSignal, 1.0e-6f))
    {
        std::cerr << "Changing module order did not change the signal as expected.\n";
        return 12;
    }

    // Each module must make an audible numerical change when enabled with a meaningful test setting.
    for (int module = 0; module < 8; ++module)
    {
        VocalStripDSP single;
        single.prepare(48000.0, 2048, 2);
        auto input = makeTestSignal(2048, 48000.0);
        juce::AudioBuffer<float> reference(2, 2048);
        reference.makeCopyOf(input, true);

        VocalStripParameters settings;
        settings.inputDb = 0.0f;
        settings.outputDb = 0.0f;
        settings.reverbPreDelayMs = 0.0f;
        settings.reverbMix = 45.0f;
        settings.delayMix = 45.0f;
        settings.delayTimeMs = 1.0f;
        settings.delayFeedback = 0.0f;
        settings.optoPeak = 100.0f;
        settings.deessFrequency = 5000.0f;
        settings.deessThresholdDb = -48.0f;
        settings.deessAmount = 100.0f;
        settings.deessRangeDb = 18.0f;
        settings.deessAttackMs = 0.1f;
        settings.consoleLowGainDb = 8.0f;
        settings.airMid = 100.0f;
        settings.airHigh = 100.0f;
        settings.fetThresholdDb = -36.0f;
        settings.fetRatio = 20.0f;
        settings.fetAttackMs = 0.1f;
        settings.fetMix = 100.0f;
        settings.peqBands[0] = { 180.0f, 9.0f, 1.0f };

        std::array<bool, 8> justOne {};
        justOne[static_cast<std::size_t>(module)] = true;
        single.process(input, settings, order, justOne);
        if (!allFinite(input) || !differsFrom(input, reference))
        {
            std::cerr << "Module " << module << " did not produce a finite, audible response.\n";
            return 4 + module;
        }
    }

    std::cout << "OFF SESSION VOCAL STRIP DSP tests passed (dry path, all eight modules, 44.1 / 48 / 96 kHz and reordered chain).\n";
    return 0;
}
