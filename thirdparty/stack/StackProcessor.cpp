#include "StackProcessor.h"
#include "StackEditor.h"
#include <json/json.hpp>

#ifdef RNBO_INCLUDE_DESCRIPTION_FILE
#include <rnbo_description.h>
#endif

using namespace stack;


StackProcessor::StackProcessor(
    const nlohmann::json& patcher_desc,
    const nlohmann::json& presets,
    const RNBO::BinaryData& data
    )
  : RNBO::JuceAudioProcessor(patcher_desc, presets, data)
{
    magic = std::make_unique<foleys::Magic> (this);
    magic->buildFactory = [&](foleys::MagicGUIBuilder& builder) { makeFactoryWidgets(builder); };
}

//create an instance of our custom plugin, optionally set description, presets and binary data (datarefs)
StackProcessor* StackProcessor::CreateDefault() {
    nlohmann::json patcher_desc, presets;

#ifdef RNBO_BINARY_DATA_STORAGE_NAME
    extern RNBO::BinaryDataImpl::Storage RNBO_BINARY_DATA_STORAGE_NAME;
    RNBO::BinaryDataImpl::Storage dataStorage = RNBO_BINARY_DATA_STORAGE_NAME;
#else
    RNBO::BinaryDataImpl::Storage dataStorage;
#endif
    RNBO::BinaryDataImpl data(dataStorage);

#ifdef RNBO_INCLUDE_DESCRIPTION_FILE
    patcher_desc = RNBO::patcher_description;
    presets = RNBO::patcher_presets;
#endif
    return new StackProcessor(patcher_desc, presets, data);
}


juce::AudioProcessorEditor* StackProcessor::createEditor()
{
    //Change this to use your CustomAudioEditor
    //return new CustomAudioEditor (this, this->_rnboObject);
    // return RNBO::JuceAudioProcessor::createEditor();

    return magic->createEditor(magic->createGuiValueTree(getParameterTree()));
}


void StackProcessor::makeFactoryWidgets(foleys::MagicGUIBuilder& builder)
{
    magic->buildFactory = [&](foleys::MagicGUIBuilder& builder) {
        // builder.registerFactory("CustomSlider", CustomSliderContainer::factory);
    };

}

void StackProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    processPreRNBO (buffer, midiMessages);
    RNBO::JuceAudioProcessor::processBlock(buffer, midiMessages);
    processPostRNBO (buffer, midiMessages);
}
