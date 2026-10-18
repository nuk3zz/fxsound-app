#include "DeviceRoute.h"
#include "GlobalHotkey.h"
#include "PreviewProcessor.h"

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <atomic>

namespace fxsound::preview {
static_assert(std::atomic<int>::is_always_lock_free, "The audio thread needs a lock-free preset value.");
static_assert(std::atomic<bool>::is_always_lock_free, "The audio thread needs lock-free error flags.");

class PreviewComponent final : public juce::Component, private juce::AudioIODeviceCallback, private juce::Timer {
public:
    PreviewComponent() {
        juce::PropertiesFile::Options options;
        options.applicationName = "FxSound Mac Preview";
        options.filenameSuffix = ".settings";
        options.osxLibrarySubFolder = "Application Support";
        settings_.setStorageParameters(options);
        const auto alternate = settings_.getUserSettings()->getIntValue("alternatePreset", 1);
        toggle_.setAlternate(alternate == 2 ? Preset::speech : Preset::bassBoost);

        title_.setText("FxSound Mac Preview", juce::dontSendNotification);
        title_.setFont(juce::Font(juce::FontOptions(24.0f)));
        description_.setText("Small preview EQ: General, Bass Boost and Speech. Original FxSound DSP is not included.",
                             juce::dontSendNotification);
        inputLabel_.setText("Loopback input", juce::dontSendNotification);
        outputLabel_.setText("Physical output", juce::dontSendNotification);
        alternateLabel_.setText("Alternate preset", juce::dontSendNotification);
        input_.setTextWhenNothingSelected("Select BlackHole (stereo)");
        output_.setTextWhenNothingSelected("Select speakers or headphones");
        alternate_.addItem("Bass Boost", 1);
        alternate_.addItem("Speech", 2);
        alternate_.setSelectedId(static_cast<int>(toggle_.alternate()), juce::dontSendNotification);
        alternate_.onChange = [this] {
            toggle_.setAlternate(alternate_.getSelectedId() == 2 ? Preset::speech : Preset::bassBoost);
            settings_.getUserSettings()->setValue("alternatePreset", static_cast<int>(toggle_.alternate()));
            settings_.saveIfNeeded();
            publishPreset();
        };
        start_.setButtonText("Start audio");
        start_.onClick = [this] { if (running_) stopAudio("Stopped. Restore macOS sound output to your speakers/headphones."); else startAudio(); };
        refresh_.setButtonText("Refresh devices");
        refresh_.onClick = [this] { refreshDevices(); };
        toggleButton_.setButtonText("Toggle preset");
        toggleButton_.onClick = [this] { togglePreset(); };
        instructions_.setText(
            "1. Install BlackHole 2ch, then select it above as the input.\n"
            "2. Select your physical speakers/headphones above. Lower their volume for the first test.\n"
            "3. Set macOS Sound Output to BlackHole 2ch, then click Start audio.\n"
            "4. Cmd + Option + B toggles General and your alternate, including while hidden.\n"
            "5. Before stopping or quitting, restore macOS Sound Output to your physical device.\n\n"
            "This app never changes the macOS default output. Closing the window hides it; use the menu bar icon to reopen or quit.",
            juce::dontSendNotification);
        instructions_.setJustificationType(juce::Justification::topLeft);
        for (auto* component : std::initializer_list<juce::Component*> {
                &title_, &description_, &inputLabel_, &outputLabel_, &alternateLabel_, &input_, &output_,
                &alternate_, &start_, &refresh_, &toggleButton_, &current_, &hotkeyStatus_, &status_, &instructions_})
            addAndMakeVisible(component);
        status_.setText("Stopped. Select devices and start when ready.", juce::dontSendNotification);
        refreshDevices();
        publishPreset();
        const auto registered = hotkey_.install([this] { togglePreset(); });
        hotkeyStatus_.setText(registered ? "Global shortcut: Cmd + Option + B" :
            "Shortcut unavailable (already used). Toggle preset still works here.", juce::dontSendNotification);
        setSize(730, 600);
        startTimer(500);
    }

    ~PreviewComponent() override {
        stopTimer();
        hotkey_.reset();
        stopAudio({});
        settings_.saveIfNeeded();
    }

    void togglePreset() { toggle_.toggle(); publishPreset(); }

    void paint(juce::Graphics& graphics) override {
        graphics.fillAll(juce::Colour(0xff20252d));
    }

    void resized() override {
        auto area = getLocalBounds().reduced(22);
        title_.setBounds(area.removeFromTop(36));
        description_.setBounds(area.removeFromTop(44));
        auto inputRow = area.removeFromTop(38);
        inputLabel_.setBounds(inputRow.removeFromLeft(135)); input_.setBounds(inputRow.reduced(0, 4));
        auto outputRow = area.removeFromTop(38);
        outputLabel_.setBounds(outputRow.removeFromLeft(135)); output_.setBounds(outputRow.reduced(0, 4));
        auto alternateRow = area.removeFromTop(38);
        alternateLabel_.setBounds(alternateRow.removeFromLeft(135)); alternate_.setBounds(alternateRow.reduced(0, 4));
        area.removeFromTop(8);
        auto buttons = area.removeFromTop(34);
        start_.setBounds(buttons.removeFromLeft(160)); buttons.removeFromLeft(10);
        refresh_.setBounds(buttons.removeFromLeft(160)); buttons.removeFromLeft(10);
        toggleButton_.setBounds(buttons.removeFromLeft(160));
        current_.setBounds(area.removeFromTop(30));
        hotkeyStatus_.setBounds(area.removeFromTop(28));
        status_.setBounds(area.removeFromTop(48));
        area.removeFromTop(8);
        instructions_.setBounds(area);
    }

private:
    void publishPreset() {
        desiredPreset_.store(static_cast<int>(toggle_.current()), std::memory_order_relaxed);
        const auto name = toggle_.current() == Preset::general ? "General" :
            toggle_.current() == Preset::bassBoost ? "Bass Boost" : "Speech";
        current_.setText(juce::String("Current preset: ") + name, juce::dontSendNotification);
    }

    void refreshDevices() {
        if (running_) return;
        inputDevices_.clear(); outputDevices_.clear();
        input_.clear(juce::dontSendNotification); output_.clear(juce::dontSendNotification);
        for (const auto& device : enumerateDevices()) {
            if (device.isBlackHoleInput()) {
                inputDevices_.push_back(device); input_.addItem(device.name, static_cast<int>(inputDevices_.size()));
            }
            if (device.isPhysicalOutput()) {
                outputDevices_.push_back(device); output_.addItem(device.name, static_cast<int>(outputDevices_.size()));
            }
        }
        // No device is implicitly selected or opened, even on first launch.
        if (inputDevices_.empty()) status_.setText("No stereo BlackHole input found. Install BlackHole 2ch, then Refresh.",
                                                                  juce::dontSendNotification);
    }

    void startAudio() {
        const auto inputIndex = input_.getSelectedId() - 1;
        const auto outputIndex = output_.getSelectedId() - 1;
        if (inputIndex < 0 || outputIndex < 0 || inputIndex >= static_cast<int>(inputDevices_.size())
            || outputIndex >= static_cast<int>(outputDevices_.size())) {
            status_.setText("Choose both BlackHole input and a physical output first.", juce::dontSendNotification); return;
        }
        selectedInput_ = inputDevices_[static_cast<std::size_t>(inputIndex)];
        selectedOutput_ = outputDevices_[static_cast<std::size_t>(outputIndex)];
        if (const auto error = validateRoute(selectedInput_, selectedOutput_); error.isNotEmpty()) {
            status_.setText(error, juce::dontSendNotification); return;
        }
        // Own the explicit device instead of AudioDeviceManager: that manager may
        // automatically open default devices after hotplug even with fallback disabled.
        deviceType_.reset(juce::AudioIODeviceType::createAudioIODeviceType_CoreAudio());
        if (deviceType_ == nullptr) {
            status_.setText("CoreAudio is unavailable.", juce::dontSendNotification); return;
        }
        deviceType_->scanForDevices();
        if (!deviceType_->getDeviceNames(true).contains(selectedInput_.name)
            || !deviceType_->getDeviceNames(false).contains(selectedOutput_.name)) {
            stopAudio("Selected devices are unavailable to CoreAudio. Refresh and retry."); return;
        }
        audioDevice_.reset(deviceType_->createDevice(selectedOutput_.name, selectedInput_.name));
        if (audioDevice_ == nullptr) {
            stopAudio("CoreAudio could not create the selected route."); return;
        }
        juce::BigInteger stereo;
        stereo.setRange(0, 2, true);
        deviceError_.store(false, std::memory_order_relaxed);
        routeAllowed_.store(false, std::memory_order_relaxed);
        const auto error = audioDevice_->open(stereo, stereo, 48000, 512);
        if (error.isNotEmpty()) { stopAudio("Could not start: " + error); return; }
        if (const auto routeError = openedRouteError(); routeError.isNotEmpty()) {
            stopAudio(routeError); return;
        }
        audioDevice_->start(this);
        if (!audioDevice_->isPlaying() || deviceError_.load(std::memory_order_relaxed)) {
            stopAudio("Audio could not start. Check input privacy permission and device availability."); return;
        }
        if (const auto routeError = openedRouteError(); routeError.isNotEmpty()) {
            stopAudio(routeError); return;
        }
        routeAllowed_.store(true, std::memory_order_relaxed);
        running_ = true;
        input_.setEnabled(false); output_.setEnabled(false); refresh_.setEnabled(false);
        start_.setButtonText("Stop audio");
        status_.setText("Processing BlackHole to " + selectedOutput_.name + ". Restore macOS output before stopping.",
                         juce::dontSendNotification);
    }

    juce::String openedRouteError() const {
        if (const auto error = validateRoute(selectedInput_, selectedOutput_); error.isNotEmpty()) return error;
        if (audioDevice_ == nullptr || deviceType_ == nullptr || !audioDevice_->isOpen())
            return "The selected audio route is no longer open. Refresh devices and retry.";
        const auto inputNames = deviceType_->getDeviceNames(true);
        const auto outputNames = deviceType_->getDeviceNames(false);
        const auto inputIndex = deviceType_->getIndexOfDevice(audioDevice_.get(), true);
        const auto outputIndex = deviceType_->getIndexOfDevice(audioDevice_.get(), false);
        if (inputIndex < 0 || inputIndex >= inputNames.size() || outputIndex < 0 || outputIndex >= outputNames.size()
            || inputNames[inputIndex] != selectedInput_.name || outputNames[outputIndex] != selectedOutput_.name
            || audioDevice_->getActiveInputChannels().countNumberOfSetBits() != 2
            || audioDevice_->getActiveOutputChannels().countNumberOfSetBits() != 2)
            return "The opened route differs from the selected stereo devices. Audio stopped; refresh devices.";
        return {};
    }

    void stopAudio(const juce::String& reason) {
        running_ = false;
        routeAllowed_.store(false, std::memory_order_relaxed);
        if (audioDevice_ != nullptr) { audioDevice_->stop(); audioDevice_->close(); audioDevice_.reset(); }
        deviceType_.reset();
        input_.setEnabled(true); output_.setEnabled(true); refresh_.setEnabled(true);
        start_.setButtonText("Start audio");
        if (reason.isNotEmpty()) status_.setText(reason, juce::dontSendNotification);
    }

    void timerCallback() override {
        if (!running_) return;
        if (deviceError_.exchange(false, std::memory_order_relaxed) || !deviceLive_.load(std::memory_order_relaxed)
            || !routeAllowed_.load(std::memory_order_relaxed)) {
            stopAudio("Audio device stopped or failed. Restore macOS output, then Refresh and retry."); return;
        }
        if (const auto error = openedRouteError(); error.isNotEmpty())
            stopAudio(error + " Restore macOS sound output to a physical device.");
    }

    void audioDeviceIOCallbackWithContext(const float* const* input, int inputs, float* const* output,
                                          int outputs, int samples, const juce::AudioIODeviceCallbackContext&) override {
        if (routeAllowed_.load(std::memory_order_relaxed) && !deviceError_.load(std::memory_order_relaxed)) {
            processor_.setTarget(static_cast<Preset>(desiredPreset_.load(std::memory_order_relaxed)));
            processor_.process(input, inputs, output, outputs, samples);
        } else {
            for (int channel = 0; channel < outputs; ++channel)
                if (output[channel] != nullptr) std::fill_n(output[channel], samples, 0.0f);
        }
    }
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override {
        // A backend restart requires an explicit UI start and route validation again.
        routeAllowed_.store(false, std::memory_order_relaxed);
        processor_.prepare(device->getCurrentSampleRate());
        deviceLive_.store(true, std::memory_order_relaxed);
    }
    void audioDeviceStopped() override {
        routeAllowed_.store(false, std::memory_order_relaxed);
        deviceLive_.store(false, std::memory_order_relaxed);
    }
    void audioDeviceError(const juce::String&) override {
        routeAllowed_.store(false, std::memory_order_relaxed);
        deviceError_.store(true, std::memory_order_relaxed);
    }

    juce::ApplicationProperties settings_;
    std::unique_ptr<juce::AudioIODeviceType> deviceType_;
    std::unique_ptr<juce::AudioIODevice> audioDevice_;
    PreviewProcessor processor_;
    PresetToggle toggle_;
    GlobalHotkey hotkey_;
    std::atomic<int> desiredPreset_ {0};
    std::atomic<bool> deviceError_ {false}, deviceLive_ {false}, routeAllowed_ {false};
    bool running_ = false;
    std::vector<Device> inputDevices_, outputDevices_;
    Device selectedInput_, selectedOutput_;
    juce::Label title_, description_, inputLabel_, outputLabel_, alternateLabel_, current_, hotkeyStatus_, status_, instructions_;
    juce::ComboBox input_, output_, alternate_;
    juce::TextButton start_, refresh_, toggleButton_;
};

class MainWindow final : public juce::DocumentWindow {
public:
    MainWindow() : DocumentWindow("FxSound Mac Preview", juce::Colour(0xff20252d), allButtons) {
        setUsingNativeTitleBar(true);
        setContentOwned(new PreviewComponent(), true);
        setResizable(false, false);
        centreWithSize(getWidth(), getHeight());
        setVisible(true);
    }
    void closeButtonPressed() override { setVisible(false); }
    void reopen() { setVisible(true); toFront(true); }
    void togglePreset() { static_cast<PreviewComponent*>(getContentComponent())->togglePreset(); }
};

class MenuBarIcon final : public juce::SystemTrayIconComponent {
public:
    MenuBarIcon(std::function<void()> show, std::function<void()> toggle)
        : show_(std::move(show)), toggle_(std::move(toggle)) {
        juce::Image icon(juce::Image::ARGB, 22, 22, true);
        juce::Graphics graphics(icon);
        graphics.setColour(juce::Colours::black);
        graphics.fillRoundedRectangle(3, 6, 3, 10, 1);
        graphics.fillRoundedRectangle(9, 2, 3, 18, 1);
        graphics.fillRoundedRectangle(15, 5, 3, 12, 1);
        setIconImage(icon, icon);
        setIconTooltip("FxSound Mac Preview");
    }
    void mouseUp(const juce::MouseEvent&) override {
        juce::PopupMenu menu;
        menu.addItem(1, "Show FxSound Mac Preview");
        menu.addItem(2, "Toggle General / alternate");
        menu.addSeparator();
        menu.addItem(3, "Quit (restore macOS sound output first)");
        const juce::Component::SafePointer<MenuBarIcon> safe(this);
        menu.showMenuAsync({}, [safe](int result) {
            if (safe == nullptr) return;
            if (result == 1) safe->show_();
            if (result == 2) safe->toggle_();
            if (result == 3) juce::JUCEApplication::getInstance()->systemRequestedQuit();
        });
    }
private:
    std::function<void()> show_, toggle_;
};

class Application final : public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override { return "FxSound Mac Preview"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override { return false; }
    void initialise(const juce::String&) override {
        window_ = std::make_unique<MainWindow>();
        icon_ = std::make_unique<MenuBarIcon>([this] { window_->reopen(); }, [this] { window_->togglePreset(); });
    }
    void shutdown() override { icon_.reset(); window_.reset(); }
    void systemRequestedQuit() override { quit(); }
    void anotherInstanceStarted(const juce::String&) override { if (window_) window_->reopen(); }
private:
    std::unique_ptr<MainWindow> window_;
    std::unique_ptr<MenuBarIcon> icon_;
};
} // namespace fxsound::preview

START_JUCE_APPLICATION(fxsound::preview::Application)
