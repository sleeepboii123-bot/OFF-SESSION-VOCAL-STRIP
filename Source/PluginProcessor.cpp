#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <vector>

namespace
{
using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;
using FloatParam = juce::AudioParameterFloat;
using ChoiceParam = juce::AudioParameterChoice;
using BoolParam = juce::AudioParameterBool;
using IntParam = juce::AudioParameterInt;

void addFloat(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& p,
              const char* id, const char* name, float minimum, float maximum,
              float initial, const char* suffix = "", float skew = 1.0f)
{
    p.push_back(std::make_unique<FloatParam>(juce::ParameterID { id, 1 }, name,
        juce::NormalisableRange<float>(minimum, maximum, 0.01f, skew), initial, suffix));
}

void addBool(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& p,
             const char* id, const char* name, bool initial)
{
    p.push_back(std::make_unique<BoolParam>(juce::ParameterID { id, 1 }, name, initial));
}

float safePeak(float p)
{
    return std::isfinite(p) ? juce::jmax(0.0f, p) : 0.0f;
}
}

Layout OffSessionVocalStripAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    addFloat(p, "input_db", "Input", -24.0f, 24.0f, 0.0f, " dB");
    addFloat(p, "output_db", "Output", -24.0f, 18.0f, 0.0f, " dB");
    addBool(p, "global_bypass", "Global Bypass", false);

    const char* enableIds[] = { "enable_opto", "enable_peq", "enable_deess", "enable_console",
                                "enable_air", "enable_fet", "enable_reverb", "enable_delay" };
    const char* enableNames[] = { "Opto Enabled", "Parametric EQ Enabled", "De-Esser Enabled",
                                  "Console EQ Enabled", "Air Enabled", "FET Enabled",
                                  "Reverb Enabled", "Delay Enabled" };
    for (int i = 0; i < 8; ++i)
        addBool(p, enableIds[i], enableNames[i], true);

    for (int i = 0; i < 8; ++i)
        p.push_back(std::make_unique<IntParam>(juce::ParameterID { "slot" + juce::String(i), 1 },
            "Chain Position " + juce::String(i + 1), 0, 7, i));

    // 01 - Optical leveller. Attack/release remain program-dependent as on classic opto units.
    addFloat(p, "opto_peak", "Peak Reduction", 0.0f, 100.0f, 42.0f, "%");
    addFloat(p, "opto_gain", "Gain", -12.0f, 24.0f, 2.0f, " dB");
    addFloat(p, "opto_emphasis", "Emphasis", 0.0f, 100.0f, 25.0f, "%");
    addFloat(p, "opto_mix", "Mix", 0.0f, 100.0f, 100.0f, "%");
    p.push_back(std::make_unique<ChoiceParam>(juce::ParameterID { "opto_mode", 1 }, "Opto Mode",
        juce::StringArray { "Compress", "Limit" }, 0));
    p.push_back(std::make_unique<ChoiceParam>(juce::ParameterID { "opto_meter_mode", 1 }, "Opto Meter",
        juce::StringArray { "Gain Reduction", "Output" }, 0));

    // 02 - Four fully parametric bands plus variable HP / LP filters.
    addFloat(p, "peq_hp", "High Pass", 20.0f, 1000.0f, 45.0f, " Hz", 0.35f);
    addFloat(p, "peq_lp", "Low Pass", 2000.0f, 20000.0f, 20000.0f, " Hz", 0.45f);
    const float freqDefaults[] = { 180.0f, 450.0f, 2800.0f, 9500.0f };
    for (int i = 0; i < 4; ++i)
    {
        const auto n = juce::String(i + 1);
        const auto fId = "peq_f" + n;
        const auto gId = "peq_g" + n;
        const auto qId = "peq_q" + n;
        const auto fName = "Band " + n + " Frequency";
        const auto gName = "Band " + n + " Gain";
        const auto qName = "Band " + n + " Q";
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { fId, 1 }, fName,
            juce::NormalisableRange<float>(20.0f, 20000.0f, 0.01f, 0.35f), freqDefaults[i], " Hz"));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { gId, 1 }, gName,
            juce::NormalisableRange<float>(-18.0f, 18.0f, 0.01f), 0.0f, " dB"));
        p.push_back(std::make_unique<FloatParam>(juce::ParameterID { qId, 1 }, qName,
            juce::NormalisableRange<float>(0.2f, 8.0f, 0.01f, 0.55f), 0.8f, " Q"));
    }

    // 03 - Adaptive high-frequency detector.
    addFloat(p, "deess_freq", "Frequency", 2500.0f, 14000.0f, 6500.0f, " Hz", 0.45f);
    addFloat(p, "deess_threshold", "Threshold", -48.0f, 0.0f, -24.0f, " dB");
    addFloat(p, "deess_amount", "Amount", 0.0f, 100.0f, 55.0f, "%");
    addFloat(p, "deess_attack", "Attack", 0.1f, 20.0f, 1.5f, " ms", 0.45f);
    addFloat(p, "deess_release", "Release", 10.0f, 300.0f, 80.0f, " ms", 0.45f);
    addFloat(p, "deess_range", "Max Reduction", 0.0f, 18.0f, 8.0f, " dB");
    p.push_back(std::make_unique<ChoiceParam>(juce::ParameterID { "deess_mode", 1 }, "De-Esser Mode",
        juce::StringArray { "Split Band", "Wide Band" }, 0));

    // 04 - Musical four-band console-style EQ.
    addFloat(p, "console_low_f", "Low Frequency", 30.0f, 350.0f, 100.0f, " Hz", 0.4f);
    addFloat(p, "console_low_g", "Low Gain", -12.0f, 12.0f, 0.0f, " dB");
    addFloat(p, "console_lm_f", "Low-Mid Frequency", 100.0f, 1200.0f, 300.0f, " Hz", 0.4f);
    addFloat(p, "console_lm_g", "Low-Mid Gain", -12.0f, 12.0f, 0.0f, " dB");
    addFloat(p, "console_hm_f", "High-Mid Frequency", 600.0f, 9000.0f, 3000.0f, " Hz", 0.4f);
    addFloat(p, "console_hm_g", "High-Mid Gain", -12.0f, 12.0f, 0.0f, " dB");
    addFloat(p, "console_high_f", "High Frequency", 2500.0f, 18000.0f, 10000.0f, " Hz", 0.45f);
    addFloat(p, "console_high_g", "High Gain", -12.0f, 12.0f, 0.0f, " dB");
    addFloat(p, "console_drive", "Console Drive", 0.0f, 100.0f, 8.0f, "%");

    // 05 - Original air / presence harmonic enhancer.
    addFloat(p, "air_mid", "Mid Air", 0.0f, 100.0f, 22.0f, "%");
    addFloat(p, "air_high", "High Air", 0.0f, 100.0f, 26.0f, "%");
    addFloat(p, "air_tone", "Tone", 0.0f, 100.0f, 55.0f, "%");
    addFloat(p, "air_trim", "Trim", -12.0f, 12.0f, 0.0f, " dB");

    // 06 - Fast FET-style compressor.
    addFloat(p, "fet_threshold", "Threshold", -36.0f, 0.0f, -18.0f, " dB");
    addFloat(p, "fet_ratio", "Ratio", 2.0f, 20.0f, 4.0f, " :1");
    addFloat(p, "fet_attack", "Attack", 0.1f, 30.0f, 2.0f, " ms", 0.45f);
    addFloat(p, "fet_release", "Release", 10.0f, 500.0f, 80.0f, " ms", 0.45f);
    addFloat(p, "fet_makeup", "Makeup", -6.0f, 18.0f, 0.0f, " dB");
    addFloat(p, "fet_mix", "Parallel Mix", 0.0f, 100.0f, 100.0f, "%");

    // 07 - Algorithmic reverb. No convolution or partitioned processing.
    addFloat(p, "reverb_mix", "Mix", 0.0f, 100.0f, 12.0f, "%");
    addFloat(p, "reverb_size", "Size", 0.0f, 100.0f, 42.0f, "%");
    addFloat(p, "reverb_decay", "Decay", 0.1f, 8.0f, 1.6f, " s", 0.5f);
    addFloat(p, "reverb_predelay", "Pre-Delay", 0.0f, 240.0f, 20.0f, " ms");
    addFloat(p, "reverb_damping", "Damping", 0.0f, 100.0f, 50.0f, "%");
    addFloat(p, "reverb_width", "Width", 0.0f, 100.0f, 100.0f, "%");
    addFloat(p, "reverb_lowcut", "Low Cut", 20.0f, 1000.0f, 180.0f, " Hz", 0.4f);
    addFloat(p, "reverb_highcut", "High Cut", 3000.0f, 20000.0f, 12000.0f, " Hz", 0.45f);
    p.push_back(std::make_unique<ChoiceParam>(juce::ParameterID { "reverb_type", 1 }, "Reverb Type",
        juce::StringArray { "Room", "Plate", "Hall" }, 1));

    // 08 - Stereo / ping-pong delay with ducking on the wet return.
    addFloat(p, "delay_mix", "Mix", 0.0f, 100.0f, 0.0f, "%");
    addFloat(p, "delay_time", "Time", 1.0f, 1950.0f, 350.0f, " ms", 0.5f);
    addFloat(p, "delay_feedback", "Feedback", 0.0f, 90.0f, 25.0f, "%");
    addFloat(p, "delay_duck", "Ducking", 0.0f, 100.0f, 0.0f, "%");
    addFloat(p, "delay_lowcut", "Low Cut", 20.0f, 1000.0f, 150.0f, " Hz", 0.4f);
    addFloat(p, "delay_highcut", "High Cut", 2000.0f, 18000.0f, 10000.0f, " Hz", 0.45f);
    p.push_back(std::make_unique<ChoiceParam>(juce::ParameterID { "delay_mode", 1 }, "Delay Mode",
        juce::StringArray { "Stereo", "Ping-Pong" }, 0));

    return { p.begin(), p.end() };
}

OffSessionVocalStripAudioProcessor::OffSessionVocalStripAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "OFF_SESSION_VOCAL_STRIP_STATE", createParameterLayout())
{
    setLatencySamples(0);
}

void OffSessionVocalStripAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    dsp_.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    setLatencySamples(0);
}

void OffSessionVocalStripAudioProcessor::releaseResources()
{
    dsp_.reset();
}

bool OffSessionVocalStripAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    return input == output && (input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo());
}

offsession::VocalStripParameters OffSessionVocalStripAudioProcessor::readParameters() const noexcept
{
    offsession::VocalStripParameters p;
    const auto value = [this](const char* id, float fallback) noexcept
    {
        if (const auto* raw = parameters.getRawParameterValue(id))
            return raw->load(std::memory_order_relaxed);
        return fallback;
    };
    p.inputDb = value("input_db", 0.0f);
    p.outputDb = value("output_db", 0.0f);
    p.optoPeak = value("opto_peak", 42.0f);
    p.optoGainDb = value("opto_gain", 2.0f);
    p.optoEmphasis = value("opto_emphasis", 25.0f);
    p.optoMix = value("opto_mix", 100.0f);
    p.optoMode = static_cast<int>(value("opto_mode", 0.0f));
    p.peqHighPassHz = value("peq_hp", 45.0f);
    p.peqLowPassHz = value("peq_lp", 20000.0f);
    static constexpr const char* fIds[] = { "peq_f1", "peq_f2", "peq_f3", "peq_f4" };
    static constexpr const char* gIds[] = { "peq_g1", "peq_g2", "peq_g3", "peq_g4" };
    static constexpr const char* qIds[] = { "peq_q1", "peq_q2", "peq_q3", "peq_q4" };
    for (int i = 0; i < 4; ++i)
    {
        auto& b = p.peqBands[static_cast<std::size_t>(i)];
        b.frequency = value(fIds[i], 1000.0f);
        b.gainDb = value(gIds[i], 0.0f);
        b.q = value(qIds[i], 0.8f);
    }
    p.deessFrequency = value("deess_freq", 6500.0f);
    p.deessThresholdDb = value("deess_threshold", -24.0f);
    p.deessAmount = value("deess_amount", 55.0f);
    p.deessAttackMs = value("deess_attack", 1.5f);
    p.deessReleaseMs = value("deess_release", 80.0f);
    p.deessRangeDb = value("deess_range", 8.0f);
    p.deessMode = static_cast<int>(value("deess_mode", 0.0f));
    p.consoleLowFrequency = value("console_low_f", 100.0f);
    p.consoleLowGainDb = value("console_low_g", 0.0f);
    p.consoleLowMidFrequency = value("console_lm_f", 300.0f);
    p.consoleLowMidGainDb = value("console_lm_g", 0.0f);
    p.consoleHighMidFrequency = value("console_hm_f", 3000.0f);
    p.consoleHighMidGainDb = value("console_hm_g", 0.0f);
    p.consoleHighFrequency = value("console_high_f", 10000.0f);
    p.consoleHighGainDb = value("console_high_g", 0.0f);
    p.consoleDrive = value("console_drive", 8.0f);
    p.airMid = value("air_mid", 22.0f);
    p.airHigh = value("air_high", 26.0f);
    p.airTone = value("air_tone", 55.0f);
    p.airTrimDb = value("air_trim", 0.0f);
    p.fetThresholdDb = value("fet_threshold", -18.0f);
    p.fetRatio = value("fet_ratio", 4.0f);
    p.fetAttackMs = value("fet_attack", 2.0f);
    p.fetReleaseMs = value("fet_release", 80.0f);
    p.fetMakeupDb = value("fet_makeup", 0.0f);
    p.fetMix = value("fet_mix", 100.0f);
    p.reverbMix = value("reverb_mix", 12.0f);
    p.reverbSize = value("reverb_size", 42.0f);
    p.reverbDecay = value("reverb_decay", 1.6f);
    p.reverbPreDelayMs = value("reverb_predelay", 20.0f);
    p.reverbDamping = value("reverb_damping", 50.0f);
    p.reverbWidth = value("reverb_width", 100.0f);
    p.reverbLowCutHz = value("reverb_lowcut", 180.0f);
    p.reverbHighCutHz = value("reverb_highcut", 12000.0f);
    p.reverbType = static_cast<int>(value("reverb_type", 1.0f));
    p.delayMix = value("delay_mix", 0.0f);
    p.delayTimeMs = value("delay_time", 350.0f);
    p.delayFeedback = value("delay_feedback", 25.0f);
    p.delayDucking = value("delay_duck", 0.0f);
    p.delayLowCutHz = value("delay_lowcut", 150.0f);
    p.delayHighCutHz = value("delay_highcut", 10000.0f);
    p.delayPingPong = static_cast<int>(value("delay_mode", 0.0f));
    return p;
}

std::array<int, 8> OffSessionVocalStripAudioProcessor::readOrder() const noexcept
{
    static constexpr const char* slotIds[] = { "slot0", "slot1", "slot2", "slot3", "slot4", "slot5", "slot6", "slot7" };
    std::array<int, 8> result {};
    std::array<bool, 8> used {};
    int next = 0;
    for (int slot = 0; slot < 8; ++slot)
    {
        const auto* raw = parameters.getRawParameterValue(slotIds[slot]);
        const int candidate = raw != nullptr ? juce::jlimit(0, 7, static_cast<int>(std::round(raw->load()))) : slot;
        if (!used[static_cast<std::size_t>(candidate)])
        {
            result[static_cast<std::size_t>(slot)] = candidate;
            used[static_cast<std::size_t>(candidate)] = true;
        }
        else
        {
            while (next < 8 && used[static_cast<std::size_t>(next)]) ++next;
            result[static_cast<std::size_t>(slot)] = juce::jmin(7, next);
            if (next < 8) used[static_cast<std::size_t>(next)] = true;
        }
    }
    return result;
}

std::array<bool, 8> OffSessionVocalStripAudioProcessor::readEnabled() const noexcept
{
    static constexpr const char* ids[] = { "enable_opto", "enable_peq", "enable_deess", "enable_console",
        "enable_air", "enable_fet", "enable_reverb", "enable_delay" };
    std::array<bool, 8> result {};
    for (int i = 0; i < 8; ++i)
    {
        const auto* raw = parameters.getRawParameterValue(ids[i]);
        result[static_cast<std::size_t>(i)] = raw == nullptr || raw->load(std::memory_order_relaxed) >= 0.5f;
    }
    return result;
}

void OffSessionVocalStripAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    float inPeak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            inPeak = juce::jmax(inPeak, std::abs(data[i]));
    }
    inputPeak_.store(safePeak(inPeak), std::memory_order_relaxed);

    const auto* bypass = parameters.getRawParameterValue("global_bypass");
    if (bypass != nullptr && bypass->load(std::memory_order_relaxed) >= 0.5f)
    {
        optoGainReductionDb_.store(0.0f, std::memory_order_relaxed);
        outputPeak_.store(safePeak(inPeak), std::memory_order_relaxed);
        return;
    }

    const auto p = readParameters();
    const auto order = readOrder();
    const auto enabled = readEnabled();
    dsp_.process(buffer, p, order, enabled);
    optoGainReductionDb_.store(dsp_.optoGainReductionDb(), std::memory_order_relaxed);

    float outPeak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            outPeak = juce::jmax(outPeak, std::abs(data[i]));
    }
    outputPeak_.store(safePeak(outPeak), std::memory_order_relaxed);
}

juce::AudioProcessorEditor* OffSessionVocalStripAudioProcessor::createEditor()
{
    return new OffSessionVocalStripAudioProcessorEditor(*this);
}

void OffSessionVocalStripAudioProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    const auto state = parameters.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destination);
}

void OffSessionVocalStripAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OffSessionVocalStripAudioProcessor();
}
