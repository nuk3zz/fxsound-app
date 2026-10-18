cmake_minimum_required(VERSION 3.22)
project(FxSoundMacPreview VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(FXSOUND_BUILD_MAC_APP "Build the native macOS audio-routing preview" ${APPLE})

add_library(fxsound_preview_core INTERFACE)
target_include_directories(fxsound_preview_core INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
include(CTest)
if(BUILD_TESTING)
    add_executable(fxsound_preview_tests Tests/PreviewTests.cpp)
    target_link_libraries(fxsound_preview_tests PRIVATE fxsound_preview_core)
    if(MSVC)
        target_compile_options(fxsound_preview_tests PRIVATE /W4 /WX)
    else()
        target_compile_options(fxsound_preview_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME preview_core COMMAND fxsound_preview_tests)
endif()

if(FXSOUND_BUILD_MAC_APP)
    if(NOT APPLE)
        message(FATAL_ERROR "The native app requires macOS and Xcode. Use -DFXSOUND_BUILD_MAC_APP=OFF for portable tests.")
    endif()
    enable_language(OBJCXX)
    set(JUCE_SOURCE_DIR "" CACHE PATH "Optional local JUCE 9.0.4 source checkout")
    if(JUCE_SOURCE_DIR)
        add_subdirectory("${JUCE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")
    else()
        include(FetchContent)
        FetchContent_Declare(JUCE
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            # JUCE 9.0.4, pinned so a tag move cannot silently change the build.
            GIT_TAG 3fb5cbc0340758596dac39f7c8de47d562b4896e)
        FetchContent_MakeAvailable(JUCE)
    endif()
    juce_add_gui_app(FxSoundMacPreview
        PRODUCT_NAME "FxSound Mac Preview"
        COMPANY_NAME "FxSound Community Fork"
        BUNDLE_ID "org.fxsound.community.macpreview"
        MICROPHONE_PERMISSION_ENABLED TRUE
        MICROPHONE_PERMISSION_TEXT "Read the BlackHole loopback device you select to process system audio locally.")
    target_sources(FxSoundMacPreview PRIVATE Source/Main.cpp Source/GlobalHotkey.cpp Source/DeviceRoute.cpp)
    target_compile_definitions(FxSoundMacPreview PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(FxSoundMacPreview PRIVATE
        fxsound_preview_core juce::juce_audio_devices juce::juce_gui_extra juce::juce_data_structures
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags
        "-framework Carbon" "-framework CoreAudio")
endif()
