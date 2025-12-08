#pragma once

#include <JuceHeader.h>

struct DebugWindow : public juce::DocumentWindow
{
    DebugWindow();

    void closeButtonPressed() override;
};
