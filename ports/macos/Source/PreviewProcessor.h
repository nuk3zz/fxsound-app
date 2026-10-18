#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace fxsound::preview {

enum class Preset { general = 0, bassBoost = 1, speech = 2 };

// Message-thread state; publish only the resolved preset to the audio thread.
class PresetToggle {
public:
    void setAlternate(Preset value) noexcept {
        alternate_ = value == Preset::speech ? Preset::speech : Preset::bassBoost;
    }
    void toggle() noexcept { alternateActive_ = !alternateActive_; }
    Preset current() const noexcept { return alternateActive_ ? alternate_ : Preset::general; }
    Preset alternate() const noexcept { return alternate_; }
private:
    Preset alternate_ = Preset::bassBoost;
    bool alternateActive_ = false;
};

// A deliberately small preview EQ, not the original FxSound DSP engine.
// prepare() runs while stopped; all other methods belong to the audio thread.
// Processing uses fixed storage and performs no allocation, locking, or I/O.
class PreviewProcessor {
public:
    void prepare(double sampleRate) noexcept {
        sampleRate_ = std::isfinite(sampleRate) ? std::clamp(sampleRate, 8000.0, 384000.0) : 48000.0;
        rampSamples_ = std::max(1, static_cast<int>(sampleRate_ * 0.025));
        bassCoefficients_ = lowShelf(sampleRate_, 150.0, 6.0);
        speechHighPass_ = highPass(sampleRate_, 120.0);
        speechPresence_ = peak(sampleRate_, std::min(2500.0, sampleRate_ * 0.25), 3.0);
        for (auto& channel : channels_) channel = {};
        weights_ = {1.0, 0.0, 0.0};
        targetWeights_ = weights_;
        currentTarget_ = Preset::general;
        rampRemaining_ = 0;
    }

    void setTarget(Preset preset) noexcept {
        if (preset != Preset::bassBoost && preset != Preset::speech) preset = Preset::general;
        if (preset == currentTarget_) return;
        currentTarget_ = preset;
        targetWeights_ = {};
        targetWeights_[static_cast<std::size_t>(preset)] = 1.0;
        rampRemaining_ = rampSamples_;
    }

    void process(const float* const* inputs, int inputChannels,
                 float* const* outputs, int outputChannels, int samples) noexcept {
        for (int sample = 0; sample < samples; ++sample) {
            if (rampRemaining_ > 0) {
                for (std::size_t p = 0; p < weights_.size(); ++p)
                    weights_[p] += (targetWeights_[p] - weights_[p]) / rampRemaining_;
                --rampRemaining_;
            }
            // Run alternate filters continuously, avoiding stale history at a toggle.
            std::array<float, 2> result {};
            for (std::size_t c = 0; c < channels_.size(); ++c) {
                double dry = inputs != nullptr && static_cast<int>(c) < inputChannels && inputs[c] != nullptr
                    ? inputs[c][sample] : 0.0;
                if (!std::isfinite(dry)) { dry = 0.0; channels_[c] = {}; }
                const auto bass = channels_[c].bass.tick(dry, bassCoefficients_) * bassHeadroom;
                const auto speech = channels_[c].presence.tick(
                    channels_[c].highPass.tick(dry, speechHighPass_), speechPresence_) * speechHeadroom;
                const auto mixed = dry * weights_[0] + bass * weights_[1] + speech * weights_[2];
                result[c] = std::isfinite(mixed) ? static_cast<float>(std::clamp(mixed, -1.0, 1.0)) : 0.0f;
            }
            for (int c = 0; c < outputChannels; ++c)
                if (outputs != nullptr && outputs[c] != nullptr)
                    outputs[c][sample] = c < 2 ? result[static_cast<std::size_t>(c)] : 0.0f;
        }
    }

    int rampSamples() const noexcept { return rampSamples_; }
    double alternateMix() const noexcept { return weights_[1] + weights_[2]; }

private:
    struct Coefficients { double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };
    struct Filter {
        double z1 = 0, z2 = 0;
        double tick(double input, const Coefficients& c) noexcept {
            const auto output = c.b0 * input + z1;
            z1 = c.b1 * input - c.a1 * output + z2;
            z2 = c.b2 * input - c.a2 * output;
            if (std::abs(z1) < 1.0e-25) z1 = 0;
            if (std::abs(z2) < 1.0e-25) z2 = 0;
            return output;
        }
    };
    struct Channel { Filter bass, highPass, presence; };
    static Coefficients normalise(double b0, double b1, double b2, double a0, double a1, double a2) noexcept {
        return {b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0};
    }
    // RBJ audio EQ cookbook: shelf slope S=1, high-pass Q=1/sqrt(2), presence Q=1.
    static Coefficients lowShelf(double rate, double frequency, double gainDb) noexcept {
        const auto a = std::pow(10.0, gainDb / 40.0);
        const auto w = 2.0 * pi * frequency / rate;
        const auto cosine = std::cos(w);
        const auto beta = std::sqrt(2.0 * a) * std::sin(w);
        return normalise(a * ((a + 1) - (a - 1) * cosine + beta),
                         2 * a * ((a - 1) - (a + 1) * cosine),
                         a * ((a + 1) - (a - 1) * cosine - beta),
                         (a + 1) + (a - 1) * cosine + beta,
                         -2 * ((a - 1) + (a + 1) * cosine),
                         (a + 1) + (a - 1) * cosine - beta);
    }
    static Coefficients highPass(double rate, double frequency) noexcept {
        const auto w = 2 * pi * frequency / rate;
        const auto c = std::cos(w);
        const auto alpha = std::sin(w) / std::sqrt(2.0);
        return normalise((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + alpha, -2 * c, 1 - alpha);
    }
    static Coefficients peak(double rate, double frequency, double gainDb) noexcept {
        const auto a = std::pow(10.0, gainDb / 40.0);
        const auto w = 2 * pi * frequency / rate;
        const auto alpha = std::sin(w) / 2;
        return normalise(1 + alpha * a, -2 * std::cos(w), 1 - alpha * a,
                         1 + alpha / a, -2 * std::cos(w), 1 - alpha / a);
    }
    static constexpr double pi = 3.14159265358979323846;
    static constexpr double bassHeadroom = 0.5011872336272722; // -6 dB.
    static constexpr double speechHeadroom = 0.7079457843841379; // -3 dB.
    std::array<Channel, 2> channels_ {};
    Coefficients bassCoefficients_, speechHighPass_, speechPresence_;
    std::array<double, 3> weights_ {1.0, 0.0, 0.0}, targetWeights_ {1.0, 0.0, 0.0};
    double sampleRate_ = 48000;
    int rampSamples_ = 1200, rampRemaining_ = 0;
    Preset currentTarget_ = Preset::general;
};
} // namespace fxsound::preview
