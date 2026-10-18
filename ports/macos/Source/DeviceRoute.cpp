#include "DeviceRoute.h"
#include <CoreAudio/CoreAudio.h>
#include <cstddef>

namespace fxsound::preview {
namespace {
juce::String stringProperty(AudioDeviceID device, AudioObjectPropertySelector selector) {
    AudioObjectPropertyAddress address {selector, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
    CFStringRef value = nullptr;
    UInt32 size = sizeof(value);
    if (AudioObjectGetPropertyData(device, &address, 0, nullptr, &size, &value) != noErr || value == nullptr)
        return {};
    const auto result = juce::String::fromCFString(value);
    CFRelease(value);
    return result;
}

unsigned channels(AudioDeviceID device, AudioObjectPropertyScope scope) {
    AudioObjectPropertyAddress address {kAudioDevicePropertyStreamConfiguration, scope, kAudioObjectPropertyElementMain};
    UInt32 size = 0;
    if (AudioObjectGetPropertyDataSize(device, &address, 0, nullptr, &size) != noErr || size < sizeof(UInt32))
        return 0;
    // The vector provides alignment suitable for AudioBufferList, unlike byte storage.
    std::vector<std::max_align_t> memory((size + sizeof(std::max_align_t) - 1) / sizeof(std::max_align_t));
    auto* buffers = reinterpret_cast<AudioBufferList*>(memory.data());
    if (AudioObjectGetPropertyData(device, &address, 0, nullptr, &size, buffers) != noErr) return 0;
    unsigned result = 0;
    for (UInt32 i = 0; i < buffers->mNumberBuffers; ++i) result += buffers->mBuffers[i].mNumberChannels;
    return result;
}
} // namespace

bool Device::isBlackHoleInput() const {
    return inputs >= 2 && transport == kAudioDeviceTransportTypeVirtual
        && name.startsWithIgnoreCase("BlackHole") && uid.isNotEmpty();
}

bool Device::isPhysicalOutput() const {
    if (outputs < 2 || uid.isEmpty()) return false;
    switch (transport) {
        case kAudioDeviceTransportTypeBuiltIn:
        case kAudioDeviceTransportTypeUSB:
        case kAudioDeviceTransportTypeBluetooth:
        case kAudioDeviceTransportTypeBluetoothLE:
        case kAudioDeviceTransportTypeHDMI:
        case kAudioDeviceTransportTypeDisplayPort:
        case kAudioDeviceTransportTypeThunderbolt:
        case kAudioDeviceTransportTypeFireWire:
        case kAudioDeviceTransportTypePCI:
            return true;
        default: return false; // Excludes virtual, aggregate, AirPlay and unknown devices.
    }
}

std::vector<Device> enumerateDevices() {
    AudioObjectPropertyAddress address {kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal,
                                        kAudioObjectPropertyElementMain};
    UInt32 size = 0;
    if (AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &address, 0, nullptr, &size) != noErr) return {};
    std::vector<AudioDeviceID> ids(size / sizeof(AudioDeviceID));
    if (ids.empty() || AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, nullptr, &size, ids.data()) != noErr)
        return {};
    ids.resize(size / sizeof(AudioDeviceID));
    std::vector<Device> result;
    for (const auto id : ids) {
        Device device;
        device.id = id;
        device.name = stringProperty(id, kAudioObjectPropertyName);
        device.uid = stringProperty(id, kAudioDevicePropertyDeviceUID);
        AudioObjectPropertyAddress transport {kAudioDevicePropertyTransportType, kAudioObjectPropertyScopeGlobal,
                                              kAudioObjectPropertyElementMain};
        UInt32 transportSize = sizeof(device.transport);
        if (AudioObjectGetPropertyData(id, &transport, 0, nullptr, &transportSize, &device.transport) != noErr) continue;
        device.inputs = channels(id, kAudioObjectPropertyScopeInput);
        device.outputs = channels(id, kAudioObjectPropertyScopeOutput);
        if (device.name.isNotEmpty()) result.push_back(device);
    }
    return result;
}

juce::String validateRoute(const Device& input, const Device& output) {
    if (!input.isBlackHoleInput()) return "Choose a stereo BlackHole loopback input.";
    if (!output.isPhysicalOutput()) return "Choose a stereo physical output (built-in, USB, HDMI or Bluetooth).";
    if (input.id == output.id || input.uid == output.uid || input.name == output.name)
        return "Input and output must be different devices.";
    const auto devices = enumerateDevices();
    for (const auto* expected : {&input, &output}) {
        int matches = 0;
        for (const auto& actual : devices) {
            if (actual.name != expected->name) continue;
            ++matches;
            if (actual.id != expected->id || actual.uid != expected->uid || actual.transport != expected->transport
                || actual.inputs != expected->inputs || actual.outputs != expected->outputs)
                return "An audio device changed. Stop and refresh the device list.";
        }
        if (matches != 1) return "A selected device disappeared or its name is ambiguous. Refresh the device list.";
    }
    return {};
}
} // namespace fxsound::preview
