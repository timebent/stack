Export your RNBO code into this directory

When you do so, you will have to adjust some of the files in order for this to work with Foleys Plugin GUI Magic.

Specifically:

Find the RNBO_JuceAudioProcessor.h and the RNBO_JuceAudioProcssor.cpp in 
export->rnbo->adpaters->juce

Change the .h file by adding 
#include "JuceHeader.h"

and by changing what the class RNBO_JuceAudioProcessor inherits from ... from juce::AudioProcessor to foleys::MagicProcessor

In the .cpp files, change the constructor initialization list to use MagicProcessor instead of AudioProcessor
remove this function: bool JuceAudioProcessor::hasEditor() const
remove the function: AudioProcessorEditor* JuceAudioProcessor::createEditor()
remove the function: void JuceAudioProcessor::getStateInformation (MemoryBlock& destData)
remove the function: void JuceAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
