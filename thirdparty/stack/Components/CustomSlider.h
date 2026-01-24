#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <foleys_gui_magic/foleys_gui_magic.h>

#include "ComponentContainer.h"

struct CustomSlider : public juce::Slider {};

struct CustomSliderContainer : public ComponentContainer<juce::Slider, juce::SliderParameterAttachment>
{
    FOLEYS_DECLARE_GUI_FACTORY(CustomSliderContainer);

    CustomSliderContainer(foleys::MagicGUIBuilder& builder, const juce::ValueTree& node)
        : ComponentContainer(builder, node)
    {
        setColourTranslation({
            {"slider-background", juce::Slider::backgroundColourId},
            {"slider-thumb", juce::Slider::thumbColourId},
            {"slider-track", juce::Slider::trackColourId},
            {"rotary-fill", juce::Slider::rotarySliderFillColourId},
            {"rotary-outline", juce::Slider::rotarySliderOutlineColourId},
            {"slider-text", juce::Slider::textBoxTextColourId}
        });

        setContainedComponent(&slider);
    }
private:
    CustomSlider slider;
    std::unique_ptr<juce::SliderParameterAttachment> attachment;
};
