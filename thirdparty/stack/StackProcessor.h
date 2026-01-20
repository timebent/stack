#pragma once

#include "RNBO.h"
#include "RNBO_Utils.h"
#include "RNBO_JuceAudioProcessor.h"
#include "RNBO_BinaryData.h"
#include <json/json.hpp>
#include <juce_audio_processors/juce_audio_processors.h>

#include <foleys_gui_magic/foleys_gui_magic.h>

namespace stack {
    class StackProcessor : public RNBO::JuceAudioProcessor {
    public:
        StackProcessor::StackProcessor (const nlohmann::json& patcher_desc,
                                        const nlohmann::json& presets,
                                        const RNBO::BinaryData& data);

        static StackProcessor* CreateDefault();
        juce::AudioProcessorEditor* createEditor() override;
        void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
        virtual void processPreRNBO(juce::AudioBuffer<float>&, juce::MidiBuffer&) {};
        virtual void processPostRNBO(juce::AudioBuffer<float>&, juce::MidiBuffer&) {};
    private:
        std::unique_ptr<foleys::Magic> magic;

        void makeFactoryWidgets (foleys::MagicGUIBuilder& builder);

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StackProcessor)
    };
}