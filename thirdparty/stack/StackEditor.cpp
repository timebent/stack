#include "StackEditor.h"

using namespace stack;
StackEditor::StackEditor (RNBO::JuceAudioProcessor* const p, RNBO::CoreObject& rnboObject)
    : AudioProcessorEditor (p)
    , _rnboObject(rnboObject)
    , _audioProcessor(p)
{
    _audioProcessor->AudioProcessor::addListener(this);

    _label.setText("Hi I'm Custom Interface", juce::NotificationType::dontSendNotification);
    _label.setBounds(0, 0, 400, 300);
    _label.setColour(juce::Label::textColourId, juce::Colours::black);
    addAndMakeVisible(_label);
    setSize (_label.getWidth(), _label.getHeight());
}

StackEditor::~StackEditor()
{
    _audioProcessor->AudioProcessor::removeListener(this);
}

void StackEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::white);
}

void StackEditor::audioProcessorParameterChanged (juce::AudioProcessor*, int parameterIndex, float value)
{
    // Handle parameter changes here
}