#pragma once

#include <juce_core/juce_core.h>
#include <cstdint>
#include <vector>

namespace fxsound::preview {
struct Device {
    std::uint32_t id = 0;
    juce::String name, uid;
    std::uint32_t transport = 0;
    unsigned inputs = 0, outputs = 0;
    bool isBlackHoleInput() const;
    bool isPhysicalOutput() const;
};

std::vector<Device> enumerateDevices();
// Names must resolve uniquely to the same CoreAudio IDs and UIDs captured by UI.
// An empty return value means the route is currently safe to open/keep open.
juce::String validateRoute(const Device& input, const Device& output);
} // namespace fxsound::preview
