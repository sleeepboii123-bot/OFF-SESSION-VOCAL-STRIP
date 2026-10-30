#pragma once

#include <JuceHeader.h>
#include "VocalStripDSP.h"
#include <array>
#include <atomic>

class OffSessionVocalStripAudioProcessor final : public juce::AudioProcessor
{
public:
    OffSessionVocalStripAudioProcessor();
    ~OffSessionVocalStripAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 30.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    float inputPeak() const noexcept { return inputPeak_.load(std::memory_order_relaxed); }
    float outputPeak() const noexcept { return outputPeak_.load(std::memory_order_relaxed); }
    float optoGainReductionDb() const noexcept { return optoGainReductionDb_.load(std::memory_order_relaxed); }

private:
    offsession::VocalStripParameters readParameters() const noexcept;
    std::array<int, 8> readOrder() const noexcept;
    std::array<bool, 8> readEnabled() const noexcept;

    offsession::VocalStripDSP dsp_;
    std::atomic<float> inputPeak_ { 0.0f }, outputPeak_ { 0.0f }, optoGainReductionDb_ { 0.0f };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OffSessionVocalStripAudioProcessor)
};
