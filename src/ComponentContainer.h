#pragma once
#include "foleys_gui_magic/Layout/foleys_GuiItem.h"
#include <JuceHeader.h>

/*
 *
 * This is a base class to manage the custom component.
*/
template<typename ComponentType, typename AttachmentType = void>
struct ComponentContainer : public foleys::GuiItem
{
    using AttachmentPtr =
    std::conditional_t<std::is_void_v<AttachmentType>,
                       std::nullptr_t,
                       std::unique_ptr<AttachmentType>>;

    ComponentContainer(foleys::MagicGUIBuilder& builder,
                       const juce::ValueTree& node)
        : foleys::GuiItem(builder, node)
    {
        addAndMakeVisible(containedComponent);
    }

    std::vector<juce::Component*> getComponents() { return {containedComponent}; }
    juce::Component* getWrappedComponent() override { return containedComponent; }

    // Expose the parameter selection in Magic's editor and XML
    std::vector<foleys::SettableProperty> getSettableProperties() const override
    {
        std::vector<foleys::SettableProperty> props;
        props.push_back({ configNode, foleys::IDs::parameter, foleys::SettableProperty::Choice, {}, magicBuilder.createParameterMenuLambda() });
        return props;
    }

    // Let Magic know which parameter this item controls (reads the 'parameter' attribute)
    juce::String getControlledParameterID(juce::Point<int>) override
    {
        return configNode.getProperty(foleys::IDs::parameter, juce::String()).toString();
    }

    void update() override
    {
        jassert (containedComponent != nullptr);

        if constexpr (std::is_void_v<AttachmentType>)
        {
            // No attachment requested — do nothing
            return;
        }
        else
        {
            if (attachment == nullptr)
                return;

            auto paramID = getControlledParameterID({});
            if (paramID.isNotEmpty())
            {
                attachment = getMagicState().createAttachment(paramID, *dynamic_cast<Slider*> (containedComponent));
            }
        }
    }
    void setContainedComponent(Component* c) { containedComponent = c; }
private:
    Component* containedComponent { nullptr };
    AttachmentPtr attachment { nullptr };
};
