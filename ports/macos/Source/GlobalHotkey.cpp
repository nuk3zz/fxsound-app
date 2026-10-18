#include "GlobalHotkey.h"
#include <Carbon/Carbon.h>

namespace fxsound::preview {
namespace {
constexpr OSType signature = 0x46785350; // FxSP
constexpr UInt32 identifier = 1;
}
struct GlobalHotkey::Impl {
    EventHotKeyRef hotkey = nullptr;
    EventHandlerRef handler = nullptr;
    bool pressed = false;
    std::function<void()> callback;
    static OSStatus dispatch(EventHandlerCallRef, EventRef event, void* context) {
        EventHotKeyID id {};
        if (GetEventParameter(event, kEventParamDirectObject, typeEventHotKeyID, nullptr, sizeof(id), nullptr, &id) != noErr
            || id.signature != signature || id.id != identifier) return eventNotHandledErr;
        auto& self = *static_cast<Impl*>(context);
        if (GetEventKind(event) == kEventHotKeyReleased) self.pressed = false;
        else if (GetEventKind(event) == kEventHotKeyPressed && !self.pressed) {
            self.pressed = true;
            if (self.callback) self.callback();
        }
        return noErr;
    }
};

GlobalHotkey::GlobalHotkey() : impl_(std::make_unique<Impl>()) {}
GlobalHotkey::~GlobalHotkey() { reset(); }

bool GlobalHotkey::install(std::function<void()> callback) {
    reset();
    impl_->callback = std::move(callback);
    const EventTypeSpec events[] {{kEventClassKeyboard, kEventHotKeyPressed},
                                  {kEventClassKeyboard, kEventHotKeyReleased}};
    if (InstallApplicationEventHandler(&Impl::dispatch, 2, events, impl_.get(), &impl_->handler) != noErr) {
        reset(); return false;
    }
    const EventHotKeyID id {signature, identifier};
    if (RegisterEventHotKey(kVK_ANSI_B, cmdKey | optionKey, id, GetApplicationEventTarget(), 0, &impl_->hotkey) != noErr) {
        reset(); return false;
    }
    return true;
}

void GlobalHotkey::reset() {
    if (impl_->hotkey != nullptr) UnregisterEventHotKey(impl_->hotkey);
    if (impl_->handler != nullptr) RemoveEventHandler(impl_->handler);
    impl_->hotkey = nullptr; impl_->handler = nullptr; impl_->pressed = false; impl_->callback = {};
}
} // namespace fxsound::preview
