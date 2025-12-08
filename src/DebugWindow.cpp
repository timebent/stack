#include "DebugWindow.h"


DebugWindow::DebugWindow()
    : juce::DocumentWindow ("Debug", juce::Colours::darkgrey, juce::DocumentWindow::closeButton)
{
    // Add your debug UI here:
    setContentOwned (new juce::Label ("debug", "Debug Window"), true);

    setUsingNativeTitleBar (true);
    centreWithSize (300, 200);
    setAlwaysOnTop (true);
    setVisible (true);
}

void DebugWindow::closeButtonPressed()
{
    setVisible (false);
}