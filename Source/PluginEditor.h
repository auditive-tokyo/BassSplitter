#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class BassSplitterAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    BassSplitterAudioProcessorEditor(BassSplitterAudioProcessor&);
    ~BassSplitterAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    BassSplitterAudioProcessor& audioProcessor;

    juce::Label titleLabel;
    juce::Label freqLabel;
    juce::Label freqValueLabel;

    // クロスオーバー周波数ノブ
    juce::Slider crossoverSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> crossoverAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassSplitterAudioProcessorEditor)
};
