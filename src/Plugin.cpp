#include "MyProcessor.h"
#include "../thirdparty/stack/StackProcessor.h"

//This creates new instances of your plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
  return MyProcessor::CreateDefault();
}
