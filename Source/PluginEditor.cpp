#include "PluginProcessor.h"
#include "PluginEditor.h"

BassSplitterAudioProcessorEditor::BassSplitterAudioProcessorEditor(BassSplitterAudioProcessor& p)
    : AudioProcessorEditor(&p), 
      audioProcessor(p),
      spectrumDisplay(p.getSpectrumAnalyzer())
{
    // タイトルラベル
    titleLabel.setText("BassSplitter", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(24.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);

    // 周波数ラベル
    freqLabel.setText("Crossover Frequency", juce::dontSendNotification);
    freqLabel.setFont(juce::Font(14.0f));
    freqLabel.setJustificationType(juce::Justification::centred);
    freqLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(freqLabel);

    // 周波数値ラベル
    freqValueLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    freqValueLabel.setJustificationType(juce::Justification::centred);
    freqValueLabel.setColour(juce::Label::textColourId, juce::Colour(0xff4a90d9));
    addAndMakeVisible(freqValueLabel);

    // クロスオーバーノブ
    crossoverSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    crossoverSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    crossoverSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff4a90d9));
    crossoverSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff333344));
    crossoverSlider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    addAndMakeVisible(crossoverSlider);

    // スライダーをパラメータに接続
    crossoverAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "crossover", crossoverSlider);

    // 値変更時にラベルとスペクトラムを更新
    crossoverSlider.onValueChange = [this]() {
        int freq = static_cast<int>(crossoverSlider.getValue());
        freqValueLabel.setText(juce::String(freq) + " Hz", juce::dontSendNotification);
        spectrumDisplay.setCrossoverFrequency(static_cast<float>(freq));
    };

    // 初期値を表示
    int initialFreq = static_cast<int>(crossoverSlider.getValue());
    freqValueLabel.setText(juce::String(initialFreq) + " Hz", juce::dontSendNotification);
    spectrumDisplay.setCrossoverFrequency(static_cast<float>(initialFreq));

    // スペクトラムディスプレイ
    addAndMakeVisible(spectrumDisplay);

    setSize(500, 400);
}

BassSplitterAudioProcessorEditor::~BassSplitterAudioProcessorEditor()
{
}

void BassSplitterAudioProcessorEditor::paint(juce::Graphics& g)
{
    // 背景グラデーション
    g.fillAll(juce::Colour(0xff1a1a2e));

    // 装飾線
    g.setColour(juce::Colour(0xff4a90d9));
    g.drawRect(getLocalBounds().reduced(10), 2);
}

void BassSplitterAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);

    titleLabel.setBounds(area.removeFromTop(35));
    area.removeFromTop(5);

    // スペクトラムディスプレイ
    spectrumDisplay.setBounds(area.removeFromTop(180));
    area.removeFromTop(10);

    // ノブエリア
    auto knobArea = area;
    
    freqLabel.setBounds(knobArea.removeFromTop(20));
    knobArea.removeFromTop(5);
    
    // ノブを中央に配置
    auto knobSection = knobArea.removeFromTop(80);
    crossoverSlider.setBounds(knobSection.withSizeKeepingCentre(80, 80));

    freqValueLabel.setBounds(knobArea.removeFromTop(25));
}
