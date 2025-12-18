#include "CustomAudioProcessor.h"
#include "CustomAudioEditor.h"
#ifdef JUCE_STANDALONE_APPLICATION
#include "DebugWindow.h"
#endif
#include <json/json.hpp>

#include "CustomSlider.h"

#ifdef RNBO_INCLUDE_DESCRIPTION_FILE
#include <rnbo_description.h>
#endif

//create an instance of our custom plugin, optionally set description, presets and binary data (datarefs)
CustomAudioProcessor* CustomAudioProcessor::CreateDefault() {
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
  return new CustomAudioProcessor(patcher_desc, presets, data);
}

CustomAudioProcessor::CustomAudioProcessor(
    const nlohmann::json& patcher_desc,
    const nlohmann::json& presets,
    const RNBO::BinaryData& data
    ) 
  : RNBO::JuceAudioProcessor(patcher_desc, presets, data) 
{

}

void CustomAudioProcessor::initialiseBuilder(foleys::MagicGUIBuilder& builder)
{
    // Call parent to register standard JUCE components
    RNBO::JuceAudioProcessor::initialiseBuilder(builder);
    
    // Register our custom component
    builder.registerFactory("CustomKnob", &CustomComponents::CustomKnobItem::factory);
    builder.registerFactory("CustomSlider", CustomSliderContainer::factory);

    // Create the analyser and oscilloscope via the magic state
    analyser = magicState.createAndAddObject<foleys::MagicAnalyser>("analyser");
    oscilloscope = magicState.createAndAddObject<foleys::MagicOscilloscope>("oscilloscope");

}

void CustomAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlockExpected)
{
    // Call parent prepareToPlay
    RNBO::JuceAudioProcessor::prepareToPlay(sampleRate, samplesPerBlockExpected);
    
    // Prepare the analyser
    if (analyser)
        analyser->prepareToPlay(sampleRate, samplesPerBlockExpected);
    // Prepare the oscilloscope
    if (oscilloscope)
        oscilloscope->prepareToPlay(sampleRate, 0);

#ifdef JUCE_STANDALONE_APPLICATION
    // Notify debug window
    if (debugWindow)
        debugWindow->prepareToPlay(sampleRate, samplesPerBlockExpected);
#endif
}

void CustomAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Call parent processBlock
    RNBO::JuceAudioProcessor::processBlock(buffer, midiMessages);
    
    // Push samples to the analyser
    if (analyser)
        analyser->pushSamples(buffer);
    // Push samples to the oscilloscope
    if (oscilloscope)
     oscilloscope->pushSamples(buffer);
    
#ifdef JUCE_STANDALONE_APPLICATION
    // Push samples to debug window
    if (debugWindow)
    {
        static int logCount = 0;
        if (logCount++ == 0)
            juce::Logger::writeToLog("CustomAudioProcessor: Pushing audio to debug window");
        debugWindow->pushAudioSamples(buffer);
    }
#endif
}
