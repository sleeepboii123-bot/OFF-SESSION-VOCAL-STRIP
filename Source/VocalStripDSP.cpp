#include "VocalStripDSP.h"
#include <algorithm>
#include <cmath>

namespace offsession
{
namespace
{
constexpr double pi = 3.14159265358979323846;
constexpr float tiny = 1.0e-8f;

float clampFrequency(float hz, double sr)
{
    return juce::jlimit(20.0f, static_cast<float>(sr * 0.45), hz);
}
}

void VocalStripDSP::Biquad::set(double nb0, double nb1, double nb2,
                                double na0, double na1, double na2) noexcept
{
    if (!std::isfinite(na0) || std::abs(na0) < 1.0e-12)
        return;
    b0 = nb0 / na0;
    b1 = nb1 / na0;
    b2 = nb2 / na0;
    a1 = na1 / na0;
    a2 = na2 / na0;
    if (!std::isfinite(b0) || !std::isfinite(b1) || !std::isfinite(b2)
        || !std::isfinite(a1) || !std::isfinite(a2))
    {
        b0 = 1.0; b1 = b2 = a1 = a2 = 0.0;
        reset();
    }
}

float VocalStripDSP::Biquad::process(float x) noexcept
{
    const double y = b0 * static_cast<double>(x) + z1;
    z1 = b1 * static_cast<double>(x) - a1 * y + z2;
    z2 = b2 * static_cast<double>(x) - a2 * y;
    return std::isfinite(y) ? static_cast<float>(y) : 0.0f;
}

void VocalStripDSP::makeLowPass(Biquad& f, double sr, double hz, double q) noexcept
{
    const double w = 2.0 * pi * clampFrequency(static_cast<float>(hz), sr) / sr;
    const double c = std::cos(w), s = std::sin(w), alpha = s / (2.0 * juce::jmax(0.1, q));
    f.set((1.0 - c) * 0.5, 1.0 - c, (1.0 - c) * 0.5,
          1.0 + alpha, -2.0 * c, 1.0 - alpha);
}

void VocalStripDSP::makeHighPass(Biquad& f, double sr, double hz, double q) noexcept
{
    const double w = 2.0 * pi * clampFrequency(static_cast<float>(hz), sr) / sr;
    const double c = std::cos(w), s = std::sin(w), alpha = s / (2.0 * juce::jmax(0.1, q));
    f.set((1.0 + c) * 0.5, -(1.0 + c), (1.0 + c) * 0.5,
          1.0 + alpha, -2.0 * c, 1.0 - alpha);
}

void VocalStripDSP::makePeak(Biquad& f, double sr, double hz, double q, double gainDb) noexcept
{
    const double w = 2.0 * pi * clampFrequency(static_cast<float>(hz), sr) / sr;
    const double c = std::cos(w), s = std::sin(w);
    const double A = std::pow(10.0, juce::jlimit(-24.0, 24.0, gainDb) / 40.0);
    const double alpha = s / (2.0 * juce::jmax(0.1, q));
    f.set(1.0 + alpha * A, -2.0 * c, 1.0 - alpha * A,
          1.0 + alpha / A, -2.0 * c, 1.0 - alpha / A);
}

void VocalStripDSP::makeLowShelf(Biquad& f, double sr, double hz, double gainDb) noexcept
{
    const double w = 2.0 * pi * clampFrequency(static_cast<float>(hz), sr) / sr;
    const double c = std::cos(w), s = std::sin(w);
    const double A = std::pow(10.0, juce::jlimit(-18.0, 18.0, gainDb) / 40.0);
    const double alpha = s * 0.5 * std::sqrt(2.0);
    const double beta = 2.0 * std::sqrt(A) * alpha;
    f.set(A * ((A + 1.0) - (A - 1.0) * c + beta),
          2.0 * A * ((A - 1.0) - (A + 1.0) * c),
          A * ((A + 1.0) - (A - 1.0) * c - beta),
          (A + 1.0) + (A - 1.0) * c + beta,
          -2.0 * ((A - 1.0) + (A + 1.0) * c),
          (A + 1.0) + (A - 1.0) * c - beta);
}

void VocalStripDSP::makeHighShelf(Biquad& f, double sr, double hz, double gainDb) noexcept
{
    const double w = 2.0 * pi * clampFrequency(static_cast<float>(hz), sr) / sr;
    const double c = std::cos(w), s = std::sin(w);
    const double A = std::pow(10.0, juce::jlimit(-18.0, 18.0, gainDb) / 40.0);
    const double alpha = s * 0.5 * std::sqrt(2.0);
    const double beta = 2.0 * std::sqrt(A) * alpha;
    f.set(A * ((A + 1.0) + (A - 1.0) * c + beta),
          -2.0 * A * ((A - 1.0) + (A + 1.0) * c),
          A * ((A + 1.0) + (A - 1.0) * c - beta),
          (A + 1.0) - (A - 1.0) * c + beta,
          2.0 * ((A - 1.0) - (A + 1.0) * c),
          (A + 1.0) - (A - 1.0) * c - beta);
}

float VocalStripDSP::dbToGain(float db) noexcept
{
    return std::pow(10.0f, juce::jlimit(-90.0f, 36.0f, db) / 20.0f);
}

float VocalStripDSP::smoothCoefficient(double sampleRate, float milliseconds) noexcept
{
    const double seconds = juce::jmax(0.0001, static_cast<double>(milliseconds) * 0.001);
    return static_cast<float>(1.0 - std::exp(-1.0 / (sampleRate * seconds)));
}

void VocalStripDSP::prepare(double sampleRate, int maximumBlockSize, int channels)
{
    sampleRate_ = juce::jmax(22050.0, sampleRate);
    maxBlock_ = juce::jmax(1, maximumBlockSize);
    channels_ = juce::jlimit(1, 2, channels);
    dryScratch_.setSize(channels_, juce::jmax(maxBlock_, 16384), false, true, false);
    const auto delayCapacity = static_cast<std::size_t>(std::ceil(sampleRate_ * 2.1)) + 4U;
    const auto preDelayCapacity = static_cast<std::size_t>(std::ceil(sampleRate_ * 0.25)) + 4U;
    for (int ch = 0; ch < 2; ++ch)
    {
        delayLine_[static_cast<std::size_t>(ch)].assign(delayCapacity, 0.0f);
        preDelayLine_[static_cast<std::size_t>(ch)].assign(preDelayCapacity, 0.0f);
    }
    reverb_.reset();
    reset();
}

void VocalStripDSP::reset() noexcept
{
    optoEnvelope_ = fetEnvelope_ = deessEnvelope_ = delayDuckEnvelope_ = 0.0f;
    optoGainReductionDb_ = 0.0f;
    delayWrite_ = preDelayWrite_ = 0;
    for (int ch = 0; ch < 2; ++ch)
    {
        const auto c = static_cast<std::size_t>(ch);
        deessLowState_[c] = optoHighState_[c] = airLowState_[c] = airMidState_[c] = 0.0f;
        reverbLowState_[c] = reverbHighState_[c] = 0.0f;
        delayLowState_[c] = delayHighState_[c] = 0.0f;
        hpf_[c].reset(); lpf_[c].reset();
        for (int b = 0; b < 4; ++b)
        {
            peq_[static_cast<std::size_t>(b)][c].reset();
            console_[static_cast<std::size_t>(b)][c].reset();
        }
        std::fill(delayLine_[c].begin(), delayLine_[c].end(), 0.0f);
        std::fill(preDelayLine_[c].begin(), preDelayLine_[c].end(), 0.0f);
    }
    reverb_.reset();
}

void VocalStripDSP::process(juce::AudioBuffer<float>& buffer,
                            const VocalStripParameters& p,
                            const std::array<int, 8>& order,
                            const std::array<bool, 8>& enabled) noexcept
{
    const int channels = juce::jlimit(1, 2, buffer.getNumChannels());
    const int samples = buffer.getNumSamples();
    if (samples <= 0 || channels <= 0)
        return;

    optoGainReductionDb_ = 0.0f;
    const float inputGain = dbToGain(p.inputDb);
    for (int ch = 0; ch < channels; ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < samples; ++i)
            data[i] *= inputGain;
    }

    for (int slot = 0; slot < 8; ++slot)
    {
        const int module = juce::jlimit(0, 7, order[static_cast<std::size_t>(slot)]);
        if (!enabled[static_cast<std::size_t>(module)])
            continue;

        switch (module)
        {
            case 0: processOpto(buffer, p); break;
            case 1: processParametricEQ(buffer, p); break;
            case 2: processDeEsser(buffer, p); break;
            case 3: processConsoleEQ(buffer, p); break;
            case 4: processAir(buffer, p); break;
            case 5: processFET(buffer, p); break;
            case 6: processReverb(buffer, p); break;
            case 7: processDelay(buffer, p); break;
            default: break;
        }
    }

    const float outputGain = dbToGain(p.outputDb);
    for (int ch = 0; ch < channels; ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < samples; ++i)
        {
            const float y = data[i] * outputGain;
            data[i] = std::isfinite(y) ? juce::jlimit(-16.0f, 16.0f, y) : 0.0f;
        }
    }
}

void VocalStripDSP::processOpto(juce::AudioBuffer<float>& buffer, const VocalStripParameters& p) noexcept
{
    const int channels = juce::jmin(2, buffer.getNumChannels());
    const int samples = buffer.getNumSamples();
    const float attack = smoothCoefficient(sampleRate_, 12.0f + (100.0f - p.optoEmphasis) * 0.08f);
    const float release = smoothCoefficient(sampleRate_, 120.0f + p.optoPeak * 4.0f);
    const float thresholdDb = -4.0f - p.optoPeak * 0.43f;
    const float ratio = p.optoMode == 0 ? 3.5f : 10.0f;
    const float mix = juce::jlimit(0.0f, 1.0f, p.optoMix * 0.01f);
    const float makeup = dbToGain(p.optoGainDb);
    const float emphasis = p.optoEmphasis * 0.01f;
    const float hfAlpha = 1.0f - std::exp(-2.0f * static_cast<float>(pi) * 2200.0f / static_cast<float>(sampleRate_));

    float maximumReduction = 0.0f;
    for (int i = 0; i < samples; ++i)
    {
        float detector = 0.0f;
        for (int ch = 0; ch < channels; ++ch)
        {
            const auto c = static_cast<std::size_t>(ch);
            const float x = buffer.getReadPointer(ch)[i];
            const float low = optoHighState_[c] + hfAlpha * (x - optoHighState_[c]);
            optoHighState_[c] = low;
            const float high = x - low;
            detector = juce::jmax(detector, std::abs(x) * (1.0f - 0.15f * emphasis)
                                       + std::abs(high) * 0.9f * emphasis);
        }
        const float coeff = detector > optoEnvelope_ ? attack : release;
        optoEnvelope_ += coeff * (detector - optoEnvelope_);
        const float levelDb = 20.0f * std::log10(juce::jmax(tiny, optoEnvelope_));
        const float over = juce::jmax(0.0f, levelDb - thresholdDb);
        const float reductionDb = -over * (1.0f - 1.0f / ratio);
        maximumReduction = juce::jmax(maximumReduction, -reductionDb);
        const float gain = dbToGain(reductionDb) * makeup;
        for (int ch = 0; ch < channels; ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            const float dry = data[i];
            const float wet = dry * gain;
            data[i] = dry + (wet - dry) * mix;
        }
    }
    optoGainReductionDb_ = maximumReduction;
}

void VocalStripDSP::processParametricEQ(juce::AudioBuffer<float>& buffer, const VocalStripParameters& p) noexcept
{
    makeHighPass(hpf_[0], sampleRate_, p.peqHighPassHz, 0.707);
    makeHighPass(hpf_[1], sampleRate_, p.peqHighPassHz, 0.707);
    makeLowPass(lpf_[0], sampleRate_, p.peqLowPassHz, 0.707);
    makeLowPass(lpf_[1], sampleRate_, p.peqLowPassHz, 0.707);
    for (int band = 0; band < 4; ++band)
    {
        const auto& b = p.peqBands[static_cast<std::size_t>(band)];
        for (int ch = 0; ch < 2; ++ch)
            makePeak(peq_[static_cast<std::size_t>(band)][static_cast<std::size_t>(ch)],
                     sampleRate_, b.frequency, b.q, b.gainDb);
    }

    for (int ch = 0; ch < juce::jmin(2, buffer.getNumChannels()); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float y = hpf_[static_cast<std::size_t>(ch)].process(data[i]);
            y = lpf_[static_cast<std::size_t>(ch)].process(y);
            for (int band = 0; band < 4; ++band)
                y = peq_[static_cast<std::size_t>(band)][static_cast<std::size_t>(ch)].process(y);
            data[i] = y;
        }
    }
}

void VocalStripDSP::processDeEsser(juce::AudioBuffer<float>& buffer, const VocalStripParameters& p) noexcept
{
    const int channels = juce::jmin(2, buffer.getNumChannels());
    const float alpha = 1.0f - std::exp(-2.0f * static_cast<float>(pi)
                                        * clampFrequency(p.deessFrequency, sampleRate_) / static_cast<float>(sampleRate_));
    const float attack = smoothCoefficient(sampleRate_, p.deessAttackMs);
    const float release = smoothCoefficient(sampleRate_, p.deessReleaseMs);
    const float amount = p.deessAmount * 0.01f;
    const float maxRange = p.deessRangeDb;
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float detector = 0.0f;
        std::array<float, 2> lows {}, highs {};
        for (int ch = 0; ch < channels; ++ch)
        {
            const auto c = static_cast<std::size_t>(ch);
            const float x = buffer.getReadPointer(ch)[i];
            const float low = deessLowState_[c] + alpha * (x - deessLowState_[c]);
            deessLowState_[c] = low;
            lows[c] = low;
            highs[c] = x - low;
            detector = juce::jmax(detector, std::abs(highs[c]));
        }
        const float coeff = detector > deessEnvelope_ ? attack : release;
        deessEnvelope_ += coeff * (detector - deessEnvelope_);
        const float levelDb = 20.0f * std::log10(juce::jmax(tiny, deessEnvelope_));
        const float over = juce::jmax(0.0f, levelDb - p.deessThresholdDb);
        const float reductionDb = juce::jlimit(0.0f, maxRange, over * amount);
        const float gain = dbToGain(-reductionDb);
        for (int ch = 0; ch < channels; ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            if (p.deessMode == 0)
                data[i] = lows[static_cast<std::size_t>(ch)] + highs[static_cast<std::size_t>(ch)] * gain;
            else
                data[i] *= gain;
        }
    }
}

void VocalStripDSP::processConsoleEQ(juce::AudioBuffer<float>& buffer, const VocalStripParameters& p) noexcept
{
    for (int ch = 0; ch < 2; ++ch)
    {
        makeLowShelf(console_[0][static_cast<std::size_t>(ch)], sampleRate_, p.consoleLowFrequency, p.consoleLowGainDb);
        makePeak(console_[1][static_cast<std::size_t>(ch)], sampleRate_, p.consoleLowMidFrequency, 0.8, p.consoleLowMidGainDb);
        makePeak(console_[2][static_cast<std::size_t>(ch)], sampleRate_, p.consoleHighMidFrequency, 0.8, p.consoleHighMidGainDb);
        makeHighShelf(console_[3][static_cast<std::size_t>(ch)], sampleRate_, p.consoleHighFrequency, p.consoleHighGainDb);
    }
    const float drive = 1.0f + p.consoleDrive * 0.035f;
    const float driveMix = p.consoleDrive * 0.01f;
    for (int ch = 0; ch < juce::jmin(2, buffer.getNumChannels()); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float y = data[i];
            for (int band = 0; band < 4; ++band)
                y = console_[static_cast<std::size_t>(band)][static_cast<std::size_t>(ch)].process(y);
            if (p.consoleDrive > 0.001f)
            {
                const float saturated = std::tanh(y * drive);
                y += (saturated - y) * driveMix;
            }
            data[i] = y;
        }
    }
}

void VocalStripDSP::processAir(juce::AudioBuffer<float>& buffer, const VocalStripParameters& p) noexcept
{
    const float midCut = 1200.0f + p.airTone * 18.0f;
    const float highCut = 6200.0f + p.airTone * 55.0f;
    const float midAlpha = 1.0f - std::exp(-2.0f * static_cast<float>(pi) * midCut / static_cast<float>(sampleRate_));
    const float highAlpha = 1.0f - std::exp(-2.0f * static_cast<float>(pi) * highCut / static_cast<float>(sampleRate_));
    const float midAmount = p.airMid * 0.01f;
    const float highAmount = p.airHigh * 0.01f;
    const float trim = dbToGain(p.airTrimDb);
    for (int ch = 0; ch < juce::jmin(2, buffer.getNumChannels()); ++ch)
    {
        const auto c = static_cast<std::size_t>(ch);
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float x = data[i];
            airLowState_[c] += midAlpha * (x - airLowState_[c]);
            airMidState_[c] += highAlpha * (x - airMidState_[c]);
            const float midBand = airMidState_[c] - airLowState_[c];
            const float highBand = x - airMidState_[c];
            const float midColour = std::tanh(midBand * 2.0f) - midBand;
            const float highColour = std::tanh(highBand * 3.0f) - highBand;
            data[i] = (x + midColour * 0.36f * midAmount + highColour * 0.42f * highAmount) * trim;
        }
    }
}

void VocalStripDSP::processFET(juce::AudioBuffer<float>& buffer, const VocalStripParameters& p) noexcept
{
    const int channels = juce::jmin(2, buffer.getNumChannels());
    const float attack = smoothCoefficient(sampleRate_, p.fetAttackMs);
    const float release = smoothCoefficient(sampleRate_, p.fetReleaseMs);
    const float ratio = juce::jlimit(1.0f, 20.0f, p.fetRatio);
    const float makeup = dbToGain(p.fetMakeupDb);
    const float mix = juce::jlimit(0.0f, 1.0f, p.fetMix * 0.01f);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float detector = 0.0f;
        for (int ch = 0; ch < channels; ++ch)
            detector = juce::jmax(detector, std::abs(buffer.getReadPointer(ch)[i]));
        const float coeff = detector > fetEnvelope_ ? attack : release;
        fetEnvelope_ += coeff * (detector - fetEnvelope_);
        const float levelDb = 20.0f * std::log10(juce::jmax(tiny, fetEnvelope_));
        const float over = juce::jmax(0.0f, levelDb - p.fetThresholdDb);
        const float reductionDb = -over * (1.0f - 1.0f / ratio);
        const float gain = dbToGain(reductionDb) * makeup;
        for (int ch = 0; ch < channels; ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            const float dry = data[i];
            const float wet = dry * gain;
            data[i] = dry + (wet - dry) * mix;
        }
    }
}

void VocalStripDSP::processReverb(juce::AudioBuffer<float>& buffer, const VocalStripParameters& p) noexcept
{
    const int channels = juce::jmin(2, buffer.getNumChannels());
    const int samples = buffer.getNumSamples();
    const float mix = juce::jlimit(0.0f, 1.0f, p.reverbMix * 0.01f);
    if (mix <= 0.00001f)
        return;

    for (int ch = 0; ch < channels; ++ch)
        dryScratch_.copyFrom(ch, 0, buffer, ch, 0, samples);

    juce::Reverb::Parameters rp;
    float room = 0.12f + p.reverbSize * 0.0042f + p.reverbDecay * 0.045f;
    float damping = p.reverbDamping * 0.01f;
    if (p.reverbType == 0) // Room: compact, darker and closer.
    {
        room *= 0.72f;
        damping += 0.12f;
    }
    else if (p.reverbType == 1) // Plate: bright, broad and smooth.
    {
        room *= 1.04f;
        damping -= 0.16f;
    }
    else // Hall: larger and more diffuse.
    {
        room *= 1.22f;
        damping += 0.02f;
    }
    rp.roomSize = juce::jlimit(0.1f, 0.96f, room);
    rp.damping = juce::jlimit(0.05f, 0.95f, damping);
    rp.wetLevel = 1.0f;
    rp.dryLevel = 0.0f;
    rp.width = juce::jlimit(0.0f, 1.0f, p.reverbWidth * 0.01f);
    rp.freezeMode = 0.0f;
    reverb_.setParameters(rp);

    const float preMs = juce::jlimit(0.0f, 240.0f, p.reverbPreDelayMs);
    const auto preSamples = static_cast<std::size_t>(std::round(preMs * 0.001 * sampleRate_));
    const auto preCapacity = preDelayLine_[0].size();
    if (preSamples > 0 && preSamples < preCapacity)
    {
        for (int i = 0; i < samples; ++i)
        {
            for (int ch = 0; ch < channels; ++ch)
            {
                auto& line = preDelayLine_[static_cast<std::size_t>(ch)];
                const auto read = (preDelayWrite_ + line.size() - preSamples) % line.size();
                const float input = buffer.getReadPointer(ch)[i];
                const float delayed = line[read];
                line[preDelayWrite_] = input;
                buffer.getWritePointer(ch)[i] = delayed;
            }
            preDelayWrite_ = (preDelayWrite_ + 1U) % preCapacity;
        }
    }

    if (channels == 2)
        reverb_.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1), samples);
    else
        reverb_.processMono(buffer.getWritePointer(0), samples);

    const float lpAlpha = 1.0f - std::exp(-2.0f * static_cast<float>(pi)
        * clampFrequency(p.reverbHighCutHz, sampleRate_) / static_cast<float>(sampleRate_));
    const float hpAlpha = 1.0f - std::exp(-2.0f * static_cast<float>(pi)
        * clampFrequency(p.reverbLowCutHz, sampleRate_) / static_cast<float>(sampleRate_));

    for (int ch = 0; ch < channels; ++ch)
    {
        const auto c = static_cast<std::size_t>(ch);
        auto* wet = buffer.getWritePointer(ch);
        const auto* dry = dryScratch_.getReadPointer(ch);
        for (int i = 0; i < samples; ++i)
        {
            const float original = wet[i];
            reverbHighState_[c] += lpAlpha * (original - reverbHighState_[c]);
            const float highCut = reverbHighState_[c];
            reverbLowState_[c] += hpAlpha * (highCut - reverbLowState_[c]);
            const float filtered = highCut - reverbLowState_[c];
            wet[i] = dry[i] + (filtered - dry[i]) * mix;
        }
    }
}

void VocalStripDSP::processDelay(juce::AudioBuffer<float>& buffer, const VocalStripParameters& p) noexcept
{
    const int channels = juce::jmin(2, buffer.getNumChannels());
    const float mix = juce::jlimit(0.0f, 1.0f, p.delayMix * 0.01f);
    if (mix <= 0.00001f)
        return;

    const auto capacity = delayLine_[0].size();
    const auto delaySamples = juce::jlimit<std::size_t>(std::size_t{1}, capacity - 1U,
        static_cast<std::size_t>(std::round(juce::jlimit(1.0f, 1950.0f, p.delayTimeMs) * 0.001 * sampleRate_)));
    const float feedback = juce::jlimit(0.0f, 0.90f, p.delayFeedback * 0.01f);
    const float duckAmount = juce::jlimit(0.0f, 1.0f, p.delayDucking * 0.01f);
    const float hpAlpha = 1.0f - std::exp(-2.0f * static_cast<float>(pi)
        * clampFrequency(p.delayLowCutHz, sampleRate_) / static_cast<float>(sampleRate_));
    const float lpAlpha = 1.0f - std::exp(-2.0f * static_cast<float>(pi)
        * clampFrequency(p.delayHighCutHz, sampleRate_) / static_cast<float>(sampleRate_));
    const float duckAttack = smoothCoefficient(sampleRate_, 5.0f);
    const float duckRelease = smoothCoefficient(sampleRate_, 130.0f);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const float inputL = buffer.getReadPointer(0)[i];
        const float inputR = channels > 1 ? buffer.getReadPointer(1)[i] : inputL;
        const float detector = juce::jmax(std::abs(inputL), std::abs(inputR));
        const float duckCoeff = detector > delayDuckEnvelope_ ? duckAttack : duckRelease;
        delayDuckEnvelope_ += duckCoeff * (detector - delayDuckEnvelope_);
        const float duckGain = 1.0f - juce::jlimit(0.0f, 0.95f, delayDuckEnvelope_ * 2.0f * duckAmount);

        const auto read = (delayWrite_ + capacity - delaySamples) % capacity;
        const float wetL = delayLine_[0][read];
        const float wetR = channels > 1 ? delayLine_[1][read] : wetL;
        const float feedbackL = p.delayPingPong != 0 ? wetR : wetL;
        const float feedbackR = p.delayPingPong != 0 ? wetL : wetR;
        const float filteredL = feedbackL - delayLowState_[0];
        const float filteredR = feedbackR - delayLowState_[1];
        delayLowState_[0] += hpAlpha * (feedbackL - delayLowState_[0]);
        delayLowState_[1] += hpAlpha * (feedbackR - delayLowState_[1]);
        delayHighState_[0] += lpAlpha * (filteredL - delayHighState_[0]);
        delayHighState_[1] += lpAlpha * (filteredR - delayHighState_[1]);
        const float fbL = delayHighState_[0];
        const float fbR = delayHighState_[1];
        delayLine_[0][delayWrite_] = juce::jlimit(-8.0f, 8.0f, inputL + fbL * feedback);
        delayLine_[1][delayWrite_] = juce::jlimit(-8.0f, 8.0f, (channels > 1 ? inputR : inputL) + fbR * feedback);

        buffer.getWritePointer(0)[i] = inputL * (1.0f - mix) + wetL * mix * duckGain;
        if (channels > 1)
            buffer.getWritePointer(1)[i] = inputR * (1.0f - mix) + wetR * mix * duckGain;
        delayWrite_ = (delayWrite_ + 1U) % capacity;
    }
}
}
