#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

class PanControl : public juce::Component, public juce::Label::Listener
{
public:
    PanControl();

    void configure(juce::Colour bandColour);
    void attachToParameter(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId);

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

    // Label::Listener
    void labelTextChanged(juce::Label* labelThatHasChanged) override;
    void editorShown(juce::Label* label, juce::TextEditor& editor) override;

private:
    static constexpr float MIN_PAN_VALUE = -50.0f;
    static constexpr float MAX_PAN_VALUE = 50.0f;
    static constexpr float MIN_ANGLE = -2.75f; // radians (about -157.5 deg from top)
    static constexpr float MAX_ANGLE = 2.75f;  // radians (about +157.5 deg from top)

    juce::Slider panSlider; // invisible slider for APVTS attachment
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAttachment;
    juce::Label panValueLabel; // editable label for direct keyboard input
    juce::Colour bandColour{0xff888888};
    bool isDragging = false;
    float lastMouseY = 0.0f;

    float getCurrentValue() const;
    juce::String formatDisplayValue(float value) const;
    float getAngleFromValue(float value) const;
    void updateLabelText();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanControl)
};
