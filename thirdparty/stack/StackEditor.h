#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "RNBO.h"
#include "RNBO_JuceAudioProcessor.h"
namespace stack {
    class StackEditor : public juce::AudioProcessorEditor, private juce::AudioProcessorListener
    {
    public:
        StackEditor(RNBO::JuceAudioProcessor* const p, RNBO::CoreObject& rnboObject);
        ~StackEditor() override;
        void paint (juce::Graphics& g) override;

    private:
        void audioProcessorChanged (juce::AudioProcessor*, const ChangeDetails&) override { }
        void audioProcessorParameterChanged(juce::AudioProcessor*, int parameterIndex, float) override;

    protected:
        juce::AudioProcessor                              *_audioProcessor;
        RNBO::CoreObject&                                  _rnboObject;
        juce::Label                                       _label;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StackEditor)
    };
}