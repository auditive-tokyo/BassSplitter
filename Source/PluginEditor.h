#pragma once

#include "GUI/FaderMeter.h"
#include "GUI/SpectrumDisplay.h"
#include "PluginProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>

// 1つのバンドのUIコンポーネント
struct BandControls
{
    juce::Label nameLabel;
    juce::TextButton eqButton{"EQ"};
    juce::TextButton monoButton{"Mono"};
    juce::TextButton soloButton{"Solo"};
    juce::TextButton bypassButton{"Bypass"};
    juce::Slider panSlider;
    FaderMeter faderMeter;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> soloAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> monoAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
};

class BassSplitterAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit BassSplitterAudioProcessorEditor(BassSplitterAudioProcessor&);
    ~BassSplitterAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // ---- Private データメンバー ----
    BassSplitterAudioProcessor& audioProcessor;

    // GPU描画用OpenGLコンテキスト
    juce::OpenGLContext openGLContext;

    juce::Label titleLabel;
    juce::Label slopeLabel;

    // スロープ選択
    juce::ComboBox slopeComboBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> slopeAttachment;

    // 6バンドのコントロール
    std::array<BandControls, BassSplitterAudioProcessor::numBands> bandControls;

    // スペクトラムディスプレイ（EQOverlay内蔵）
    SpectrumDisplay spectrumDisplay;

    // ピークリセットボタン
    juce::TextButton resetPeaksButton{"Reset"};

    // ---- Private メンバー関数 ----
    void timerCallback() override;
    void setupBandControls(int bandIndex);
    void updateSpectrumDisplay();
    void updateLevelMeters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassSplitterAudioProcessorEditor)
};
