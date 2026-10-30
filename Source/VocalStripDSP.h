#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>

namespace offsession
{
struct BandParameters
{
    float frequency = 1000.0f;
    float gainDb = 0.0f;
    float q = 0.707f;
};

struct VocalStripParameters
{
    float inputDb = 0.0f;
    float outputDb = 0.0f;

    float optoPeak = 42.0f;
    float optoGainDb = 0.0f;
    float optoEmphasis = 25.0f;
    float optoMix = 100.0f;
    int optoMode = 0;

    float peqHighPassHz = 45.0f;
    float peqLowPassHz = 20000.0f;
    std::array<BandParameters, 4> peqBands {};

    float deessFrequency = 6500.0f;
    float deessThresholdDb = -24.0f;
    float deessAmount = 50.0f;
    float deessAttackMs = 1.5f;
    float deessReleaseMs = 80.0f;
    float deessRangeDb = 8.0f;
    int deessMode = 0;

    float consoleLowFrequency = 100.0f;
    float consoleLowGainDb = 0.0f;
    float consoleLowMidFrequency = 300.0f;
    float consoleLowMidGainDb = 0.0f;
    float consoleHighMidFrequency = 3000.0f;
    float consoleHighMidGainDb = 0.0f;
    float consoleHighFrequency = 10000.0f;
    float consoleHighGainDb = 0.0f;
    float consoleDrive = 0.0f;

    float airMid = 24.0f;
    float airHigh = 24.0f;
    float airTone = 55.0f;
    float airTrimDb = 0.0f;

    float fetThresholdDb = -18.0f;
    float fetRatio = 4.0f;
    float fetAttackMs = 2.0f;
    float fetReleaseMs = 80.0f;
    float fetMakeupDb = 0.0f;
    float fetMix = 100.0f;

    float reverbMix = 12.0f;
    float reverbSize = 40.0f;
    float reverbDecay = 1.5f;
    float reverbPreDelayMs = 20.0f;
    float reverbDamping = 50.0f;
    float reverbWidth = 100.0f;
    float reverbLowCutHz = 180.0f;
    float reverbHighCutHz = 12000.0f;
    int reverbType = 1;

    float delayMix = 0.0f;
    float delayTimeMs = 350.0f;
    float delayFeedback = 25.0f;
    int delayPingPong = 0;
    float delayDucking = 0.0f;
    float delayLowCutHz = 150.0f;
    float delayHighCutHz = 10000.0f;
};

class VocalStripDSP final
{
public:
    void prepare(double sampleRate, int maximumBlockSize, int channels);
    void reset() noexcept;
    float optoGainReductionDb() const noexcept { return optoGainReductionDb_; }
    void process(juce::AudioBuffer<float>& buffer,
                 const VocalStripParameters& parameters,
                 const std::array<int, 8>& order,
                 const std::array<bool, 8>& enabled) noexcept;

private:
    struct Biquad
    {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
        double z1 = 0.0, z2 = 0.0;
        void set(double nb0, double nb1, double nb2, double na0, double na1, double na2) noexcept;
        float process(float x) noexcept;
        void reset() noexcept { z1 = z2 = 0.0; }
    };

    static void makeLowPass(Biquad& f, double sr, double hz, double q = 0.707) noexcept;
    static void makeHighPass(Biquad& f, double sr, double hz, double q = 0.707) noexcept;
    static void makePeak(Biquad& f, double sr, double hz, double q, double gainDb) noexcept;
    static void makeLowShelf(Biquad& f, double sr, double hz, double gainDb) noexcept;
    static void makeHighShelf(Biquad& f, double sr, double hz, double gainDb) noexcept;
    static float dbToGain(float db) noexcept;
    static float smoothCoefficient(double sampleRate, float milliseconds) noexcept;

    void processOpto(juce::AudioBuffer<float>&, const VocalStripParameters&) noexcept;
    void processParametricEQ(juce::AudioBuffer<float>&, const VocalStripParameters&) noexcept;
    void processDeEsser(juce::AudioBuffer<float>&, const VocalStripParameters&) noexcept;
    void processConsoleEQ(juce::AudioBuffer<float>&, const VocalStripParameters&) noexcept;
    void processAir(juce::AudioBuffer<float>&, const VocalStripParameters&) noexcept;
    void processFET(juce::AudioBuffer<float>&, const VocalStripParameters&) noexcept;
    void processReverb(juce::AudioBuffer<float>&, const VocalStripParameters&) noexcept;
    void processDelay(juce::AudioBuffer<float>&, const VocalStripParameters&) noexcept;

    double sampleRate_ = 44100.0;
    int channels_ = 2;
    int maxBlock_ = 512;

    std::array<std::array<Biquad, 2>, 4> peq_ {};
    std::array<std::array<Biquad, 2>, 4> console_ {};
    std::array<Biquad, 2> hpf_ {}, lpf_ {};
    std::array<float, 2> deessLowState_ {}, optoHighState_ {}, airLowState_ {}, airMidState_ {};
    std::array<float, 2> reverbLowState_ {}, reverbHighState_ {};
    std::array<float, 2> delayLowState_ {}, delayHighState_ {};
    float optoEnvelope_ = 0.0f;
    float optoGainReductionDb_ = 0.0f;
    float fetEnvelope_ = 0.0f;
    float deessEnvelope_ = 0.0f;
    float delayDuckEnvelope_ = 0.0f;

    juce::Reverb reverb_;
    juce::AudioBuffer<float> dryScratch_;
    std::array<std::vector<float>, 2> delayLine_;
    std::array<std::vector<float>, 2> preDelayLine_;
    std::size_t delayWrite_ = 0;
    std::size_t preDelayWrite_ = 0;
};
}
