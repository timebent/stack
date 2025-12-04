### Alterations:
- These are the steps that I took to alter the repository to make this work.
- Updating the JUCE module and altering anythingn in the export folder will need to be done manually for each new RNBO export or each new iteration of the template.

1) install foleys_gui_magic by cloning the repository into the thirdparty folder.
```
git submodule add https://github.com/ffAudio/foleys_gui_magic.git
```

2) update the juce module to use version 8. 
- This was done by using
```
git pull origin master
```
  while in the juce repository, which is located in thirdparty/juce

3) Add foley to the App.cmake file 
- this has to go above juce_generate_juce_header(RNBOApp)
```
juce_add_module(thirdparty/foleys_gui_magic/modules/foleys_gui_magic)
```

- Also, add to the "target link libraries"
```
juce::juce_dsp
-- juce::juce_cryptography
-- foleys_gui_magic 
```

3a) Add foley to the Plugin.cmake file

- Add to the "target link libraries"
```
foleys_gui_magic 
```
4)  Erase this bit of code in MainComponent.cpp
 ```
        // let's listen to all midi inputs
        // enable all midi inputs
        StringArray midiInputDevices = MidiInput::getAvailableDevices();
        for (const auto& input : midiInputDevices) {
            _deviceManager.setMidiInputEnabled(input, true);
        }
        _deviceManager.addMidiInputCallback("", &_audioProcessorPlayer);
```

5) Find the RNBO_JuceAudioProcessor.h and the RNBO_JuceAudioProcssor.cpp in 
export->rnbo->adpaters->juce

- Change the .h file by adding 
```
#include "JuceHeader.h"
```

and by changing what the class RNBO_JuceAudioProcessor inherits from ... from:
```
juce::AudioProcessor
```
to 

```
foleys::MagicProcessor
```

In the .cpp files, change the constructor initialization list to use MagicProcessor instead of AudioProcessor
remove this function: 
```
bool JuceAudioProcessor::hasEditor() const
```
remove the function: 
```
AudioProcessorEditor* JuceAudioProcessor::createEditor()
```
remove the function: 
```
void JuceAudioProcessor::getStateInformation (MemoryBlock& destData)
```
remove the function: 
```
void JuceAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
```

6) In the src folder, change the customAudioProcessor.h so that you remove the following function:
```
juce::AudioProcessorEditor* createEditor()
```
7) In the src folder, change the customAudioProcessor.cpp so that you removed the following funcitons:
```
AudioProcessorEditor* CustomAudioProcessor::createEditor() 
```
