#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

class PanControl : public juce::Component
{
public:
    PanControl();

    void configure(juce::Colour bandColour);
    void attachToParameter(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId);

    void resized() override;

private:
    juce::Slider panSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanControl)
};
