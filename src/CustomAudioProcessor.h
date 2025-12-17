#include "RNBO.h"
#include "RNBO_Utils.h"
#include "RNBO_JuceAudioProcessor.h"
#include "RNBO_BinaryData.h"
#include "CustomKnobFactory.h"
#include <foleys_gui_magic/foleys_gui_magic.h>
#include <json/json.hpp>

#ifdef JUCE_STANDALONE_APPLICATION
class DebugWindow;
#endif

class CustomAudioProcessor : public RNBO::JuceAudioProcessor {
public:
    static CustomAudioProcessor* CreateDefault();
    CustomAudioProcessor(const nlohmann::json& patcher_desc, const nlohmann::json& presets, const RNBO::BinaryData& data);
    
    void initialiseBuilder(foleys::MagicGUIBuilder& builder) override;
    void prepareToPlay(double sampleRate, int samplesPerBlockExpected) override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    
#ifdef JUCE_STANDALONE_APPLICATION
    void setDebugWindow(DebugWindow* window) { debugWindow = window; }
#else
    void setDebugWindow(void* window) {}
#endif
    
private:
#ifdef JUCE_STANDALONE_APPLICATION
    DebugWindow* debugWindow = nullptr;
#endif
    
    foleys::MagicAnalyser* analyser = nullptr;
    foleys::MagicOscilloscope* oscilloscope = nullptr;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomAudioProcessor)
};

