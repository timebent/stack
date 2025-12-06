# Alterations:

## The adapters/juce files have been copied to the top level src folder. The Plugin.cmake and the App.cmake both point to there for these files.

## These are the steps that were taken to make this work. These are baked into the template.

### 0. Find the RNBO_JuceAudioProcessor.h and the RNBO_JuceAudioProcssor.cpp in 
    export->rnbo->adpaters->juce

        0a. Change the .h file by adding 
            ```
            #include "JuceHeader.h"
            ```

        0b. Also, change what class RNBO_JuceAudioProcessor inherits from ... from:
            ```
            juce::AudioProcessor
            ```
            to 

            ```
            foleys::MagicProcessor
            ```
        
        0c. Remove the createEditor() and hasEditor() declarations
            ```
            juce::AudioProcessorEditor* createEditor() override;
            bool hasEditor() const override;
            ```

        0d. Change the .cpp file. Change the constructor initialization list to use MagicProcessor instead of AudioProcessor
            ```
            JuceAudioProcessor::JuceAudioProcessor(
                    const nlohmann::json& patcher_desc,
                    const nlohmann::json& presets,
                    const RNBO::BinaryData& data,
                    JuceAudioParameterFactory* paramFactory
                )
            : CoreObjectHolder(this)
            , AudioProcessor( <<<<--------  // Change to MagicProcessor(
            ```
        
        0e. Remove these function: 
            ```
            bool JuceAudioProcessor::hasEditor() const
            ```

            ```
            AudioProcessorEditor* JuceAudioProcessor::createEditor()
            ```
        

### 1. An updated version (JUCE 8) of juce is retrieved as a submodule. The Foleys Plugin GUI Magic is also retrieved as a submodule. 

### 2. Added foley to the App.cmake file 
        - this line has to go above juce_generate_juce_header(RNBOApp)
            ```
            juce_add_module(thirdparty/foleys_gui_magic/modules/foleys_gui_magic)
            ```
        - Also, added to the "target link libraries"
            ```
            juce::juce_dsp
            juce::juce_cryptography
            foleys_gui_magic 
            ```

### 3. Added foley to the Plugin.cmake file

        - Add to the "target link libraries"
            ```
            foleys_gui_magic 
            ```

### 4.  Erased this bit of code in MainComponent.cpp
            ```
            // let's listen to all midi inputs
            // enable all midi inputs
            StringArray midiInputDevices = MidiInput::getAvailableDevices();
            for (const auto& input : midiInputDevices) {
            _deviceManager.setMidiInputEnabled(input, true);
            }
            _deviceManager.addMidiInputCallback("", &_audioProcessorPlayer);
            ```

### 5. In the src folder, changed the customAudioProcessor.h removing the following function:
            ```
            juce::AudioProcessorEditor* createEditor()
            ```
            
### 6. In the src folder, changed the customAudioProcessor.cpp so that you removed the following function:
            ```
            AudioProcessorEditor* CustomAudioProcessor::createEditor() 
            ```




