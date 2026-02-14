#include "PanControl.h"

PanControl::PanControl()
{
    panSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    panSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 14);
    panSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xff333344));
    panSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::lightgrey);
    panSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff0d0d1a));
    panSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff333344));

    addAndMakeVisible(panSlider);
}

void PanControl::configure(juce::Colour bandColour)
{
    panSlider.setColour(juce::Slider::thumbColourId, bandColour);
}

void PanControl::attachToParameter(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId)
{
    panAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, panSlider);
}

void PanControl::resized()
{
    panSlider.setBounds(getLocalBounds());
}
