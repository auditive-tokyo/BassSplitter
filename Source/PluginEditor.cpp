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

    // クロスオーバーノブ
    crossoverSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    crossoverSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 25);
    crossoverSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff4a90d9));
    crossoverSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff333344));
    crossoverSlider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    crossoverSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xff4a90d9));
    crossoverSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff0d0d1a));
    crossoverSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff333344));
    crossoverSlider.setTextValueSuffix(" Hz");
    addAndMakeVisible(crossoverSlider);

    // スライダーをパラメータに接続
    crossoverAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "crossover", crossoverSlider);

    // 値変更時にスペクトラムを更新
    crossoverSlider.onValueChange = [this]() {
        float freq = static_cast<float>(crossoverSlider.getValue());
        spectrumDisplay.setCrossoverFrequency(freq);
    };

    // 初期値でスペクトラムを更新
    spectrumDisplay.setCrossoverFrequency(static_cast<float>(crossoverSlider.getValue()));

    // スペクトラムディスプレイ
    addAndMakeVisible(spectrumDisplay);

    setSize(800, 520);
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
    spectrumDisplay.setBounds(area.removeFromTop(250));
    area.removeFromTop(10);

    // ノブエリア
    auto knobArea = area;
    
    freqLabel.setBounds(knobArea.removeFromTop(20));
    knobArea.removeFromTop(5);
    
    // ノブを中央に配置（テキストボックス分の高さも含める）
    auto knobSection = knobArea.removeFromTop(110);
    crossoverSlider.setBounds(knobSection.withSizeKeepingCentre(100, 110));
}
