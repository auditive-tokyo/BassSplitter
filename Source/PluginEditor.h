#pragma once

#include "GUI/SpectrumDisplay.h"
#include "PluginProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>

// 1つのバンドのUIコンポーネント
struct BandControls
{
    juce::Label nameLabel;
    juce::TextButton bypassButton{"B"};
    juce::TextButton soloButton{"S"};
    juce::Slider gainSlider;
    juce::Label gainLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> soloAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
};

// クロスオーバーコントロール（バンド間に配置）
struct CrossoverControl
{
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

class BassSplitterAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    BassSplitterAudioProcessorEditor(BassSplitterAudioProcessor&);
    ~BassSplitterAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setupBandControls(int bandIndex);
    void updateSpectrumDisplay();

    BassSplitterAudioProcessor& audioProcessor;

    // GPU描画用OpenGLコンテキスト
    juce::OpenGLContext openGLContext;

    juce::Label titleLabel;
    juce::Label slopeLabel;

    // スロープ選択
    juce::ComboBox slopeComboBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> slopeAttachment;

    // クロスオーバーコントロール（5つ、バンドの間に配置）
    std::array<CrossoverControl, BassSplitterAudioProcessor::numCrossovers> crossoverControls;

    // 6バンドのコントロール
    std::array<BandControls, BassSplitterAudioProcessor::numBands> bandControls;

    // スペクトラムディスプレイ
    SpectrumDisplay spectrumDisplay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassSplitterAudioProcessorEditor)
};
