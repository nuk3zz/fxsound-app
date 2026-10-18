#include "PreviewProcessor.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

using fxsound::preview::Preset;
using fxsound::preview::PresetToggle;
using fxsound::preview::PreviewProcessor;

namespace {
int failures = 0;
void check(bool condition, const char* description) {
    if (!condition) { std::cerr << "FAIL: " << description << '\n'; ++failures; }
}

void render(PreviewProcessor& processor, const std::vector<float>& input,
            std::vector<float>& left, std::vector<float>& right) {
    const float* inputs[] {input.data(), nullptr};
    float* outputs[] {left.data(), right.data()};
    processor.process(inputs, 2, outputs, 2, static_cast<int>(input.size()));
}

double gainAt(Preset preset, double rate, double frequency) {
    PreviewProcessor processor;
    processor.prepare(rate);
    processor.setTarget(preset);
    const auto size = static_cast<std::size_t>(rate);
    std::vector<float> input(size), left(size), right(size);
    for (std::size_t i = 0; i < size; ++i)
        input[i] = static_cast<float>(0.1 * std::sin(2 * 3.14159265358979323846 * frequency * static_cast<double>(i) / rate));
    render(processor, input, left, right);
    double inputPower = 0, outputPower = 0;
    for (std::size_t i = size / 2; i < size; ++i) {
        inputPower += input[i] * input[i];
        outputPower += left[i] * left[i];
    }
    return std::sqrt(outputPower / inputPower);
}

void passthroughAndChannels() {
    PreviewProcessor processor;
    processor.prepare(48000);
    std::vector<float> input {0.0f, 0.5f, -0.75f, 0.125f}, left(4), right(4), extra(4, 1.0f);
    const float* inputs[] {input.data(), nullptr};
    float* outputs[] {left.data(), right.data(), extra.data(), nullptr};
    processor.process(inputs, 2, outputs, 4, 4);
    check(left == input, "General is exact dry passthrough for normal audio");
    check(right == std::vector<float>(4), "inactive right channel stays silent");
    check(extra == std::vector<float>(4), "extra output channels are cleared");

    processor.setTarget(Preset::bassBoost);
    input.assign(10000, 0.0f); input[0] = 0.5f;
    left.resize(input.size()); right.resize(input.size());
    render(processor, input, left, right);
    check(right == std::vector<float>(input.size()), "Bass Boost keeps stereo channels isolated");
    processor.setTarget(Preset::speech);
    render(processor, input, left, right);
    check(right == std::vector<float>(input.size()), "Speech keeps stereo channels isolated");

    processor.prepare(48000);
    float* silentOutputs[] {left.data(), right.data()};
    processor.process(nullptr, 0, silentOutputs, 2, static_cast<int>(left.size()));
    check(left == std::vector<float>(left.size()) && right == left, "missing inputs produce silence");
}

void frequencyResponse() {
    for (const auto rate : {8000.0, 44100.0, 48000.0, 96000.0, 192000.0, 384000.0}) {
        const auto low = gainAt(Preset::bassBoost, rate, 30);
        const auto high = gainAt(Preset::bassBoost, rate, std::min(5000.0, rate * 0.4));
        check(low > 0.97 && low < 1.01, "Bass Boost low shelf approaches unity after headroom");
        check(high > 0.49 && high < 0.52, "Bass Boost high frequencies retain -6 dB headroom");
        check(low / high > 1.85 && low / high < 2.05, "Bass Boost provides approximately +6 dB relative bass");
        const auto rumble = gainAt(Preset::speech, rate, 30);
        const auto voice = gainAt(Preset::speech, rate, 1000);
        check(rumble < 0.06 && voice > 0.65, "Speech removes rumble and preserves voice frequencies");
        check(std::abs(gainAt(Preset::general, rate, 1000) - 1.0) < 1e-6, "General remains flat at every tested rate");
    }
}

void rampsAndState() {
    PreviewProcessor processor;
    processor.prepare(48000);
    std::vector<float> input(1, 0.1f), left(1), right(1);
    processor.setTarget(Preset::bassBoost);
    render(processor, input, left, right);
    check(std::abs(processor.alternateMix() - 1.0 / 1200.0) < 1e-12, "switch starts with one ramp step");
    for (int i = 1; i < processor.rampSamples() - 1; ++i) render(processor, input, left, right);
    check(processor.alternateMix() < 1, "fade does not finish early");
    render(processor, input, left, right);
    check(processor.alternateMix() == 1, "fade reaches exact target after 25 ms");
    processor.setTarget(Preset::general);
    for (int i = 0; i < 600; ++i) render(processor, input, left, right);
    check(std::abs(processor.alternateMix() - 0.5) < 1e-10, "return ramp reaches midpoint smoothly");
    processor.setTarget(Preset::speech);
    const auto before = processor.alternateMix();
    render(processor, input, left, right);
    check(std::abs(processor.alternateMix() - before) < 0.001, "interrupted fade continues without resetting weights");
    for (int i = 1; i < 1200; ++i) render(processor, input, left, right);
    processor.setTarget(Preset::general);
    for (int i = 0; i < 1200; ++i) render(processor, input, left, right);
    check(processor.alternateMix() == 0 && left[0] == input[0], "return to General reaches exact dry audio");
    processor.prepare(44100);
    check(processor.rampSamples() == 1102 && processor.alternateMix() == 0, "restart clears history and starts dry");

    PresetToggle toggle;
    check(toggle.current() == Preset::general, "toggle starts at General");
    toggle.toggle(); check(toggle.current() == Preset::bassBoost, "first press selects alternate");
    toggle.setAlternate(Preset::speech);
    check(toggle.current() == Preset::speech, "changing active alternate applies selected preset");
    toggle.toggle(); check(toggle.current() == Preset::general, "second press returns General");
    toggle.toggle(); check(toggle.current() == Preset::speech, "subsequent press restores chosen alternate");
    toggle.setAlternate(Preset::general);
    check(toggle.alternate() == Preset::bassBoost, "invalid alternate falls back deterministically");
}

void finiteAndStable() {
    for (const auto rate : {8000.0, 44100.0, 48000.0, 192000.0, 384000.0,
                            std::numeric_limits<double>::quiet_NaN(), -1.0}) {
        for (const auto preset : {Preset::general, Preset::bassBoost, Preset::speech}) {
            PreviewProcessor processor;
            processor.prepare(rate); processor.setTarget(preset);
            std::vector<float> input(30000, 0.25f), left(input.size()), right(input.size());
            input[1] = std::numeric_limits<float>::infinity();
            input[2] = std::numeric_limits<float>::quiet_NaN();
            input[3] = std::numeric_limits<float>::max();
            render(processor, input, left, right);
            for (const auto value : left)
                if (!std::isfinite(value) || std::abs(value) > 1) { check(false, "output remains finite and bounded"); break; }
            input.assign(input.size(), 0.0f);
            for (int block = 0; block < 4; ++block) render(processor, input, left, right);
            check(std::abs(left.back()) < 1e-10, "filter impulse/tail decays back to silence");
        }
    }
}
} // namespace

int main() {
    passthroughAndChannels(); frequencyResponse(); rampsAndState(); finiteAndStable();
    if (failures != 0) return EXIT_FAILURE;
    std::cout << "Preview core: frequency response, channels, ramps, toggle, stability passed.\n";
    return EXIT_SUCCESS;
}
