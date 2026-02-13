#pragma once

#include "../PluginProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>

/**
 * EQPanel - Each band's Highpass/Lowpass frequency controls
 * Overlays on top of the Spectrum Display
 */
class EQPanel : public juce::Component
{
public:
    EQPanel(BassSplitterAudioProcessor&);
    ~EQPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void setSelectedBand(int bandIndex);
    int getSelectedBand() const { return selectedBandIndex; }

private:
    BassSplitterAudioProcessor& audioProcessor;
    int selectedBandIndex{0};

    // 6チャンネル分のスライダー
    std::array<juce::Slider, BassSplitterAudioProcessor::numBands> highpassSliders;
    std::array<juce::Slider, BassSplitterAudioProcessor::numBands> lowpassSliders;
    std::array<juce::Label, BassSplitterAudioProcessor::numBands> bandLabels;
    std::array<juce::Label, BassSplitterAudioProcessor::numBands> highpassLabels;
    std::array<juce::Label, BassSplitterAudioProcessor::numBands> lowpassLabels;

    // Attachments
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, BassSplitterAudioProcessor::numBands> highpassAttachments;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, BassSplitterAudioProcessor::numBands> lowpassAttachments;

    // バンド選択用ボタン（タブ的な見た目）
    std::array<juce::TextButton, BassSplitterAudioProcessor::numBands> bandButtons;

    void setupControls();
    void updateBandColor();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EQPanel)
};
