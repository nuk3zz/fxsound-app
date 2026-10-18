#pragma once

#include <functional>
#include <memory>

namespace fxsound::preview {
class GlobalHotkey {
public:
    GlobalHotkey();
    ~GlobalHotkey();
    bool install(std::function<void()> callback);
    void reset();
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace fxsound::preview
