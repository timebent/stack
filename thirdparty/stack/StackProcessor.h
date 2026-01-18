#pragma once

#include "RNBO.h"
#include "RNBO_Utils.h"
#include "RNBO_JuceAudioProcessor.h"
#include "RNBO_BinaryData.h"
#include <json/json.hpp>
#include <juce_audio_processors/juce_audio_processors.h>

namespace stack {
    class StackProcessor : public RNBO::JuceAudioProcessor {
    public:
        static StackProcessor* CreateDefault();
        StackProcessor(const nlohmann::json& patcher_desc, const nlohmann::json& presets, const RNBO::BinaryData& data);
        juce::AudioProcessorEditor* createEditor() override;
    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StackProcessor)
    };
}