#include "PluginProcessor.h"
#include "PluginEditor.h"

BassSplitterAudioProcessorEditor::BassSplitterAudioProcessorEditor(BassSplitterAudioProcessor& p)
    : AudioProcessorEditor(&p), 
      audioProcessor(p),
      spectrumDisplay(p.getSpectrumAnalyzer())
{
    // GPU描画を有効化
    openGLContext.setComponentPaintingEnabled(true);
    openGLContext.attachTo(*this);

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

    // スロープラベル
    slopeLabel.setText("Slope", juce::dontSendNotification);
    slopeLabel.setFont(juce::Font(14.0f));
    slopeLabel.setJustificationType(juce::Justification::centred);
    slopeLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(slopeLabel);

    // スロープコンボボックス
    slopeComboBox.addItem("12 dB/oct", 1);
    slopeComboBox.addItem("24 dB/oct", 2);
    slopeComboBox.addItem("48 dB/oct", 3);
    slopeComboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff0d0d1a));
    slopeComboBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xff4a90d9));
    slopeComboBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff333344));
    slopeComboBox.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xff4a90d9));
    addAndMakeVisible(slopeComboBox);

    // コンボボックスをパラメータに接続
    slopeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), "slope", slopeComboBox);

    // 値変更時にスペクトラムを更新
    crossoverSlider.onValueChange = [this]() {
        float freq = static_cast<float>(crossoverSlider.getValue());
        spectrumDisplay.setCrossoverFrequency(freq);
    };

    slopeComboBox.onChange = [this]() {
        int slopeDB = audioProcessor.getCurrentSlopeDB();
        spectrumDisplay.setSlope(slopeDB);
    };

    // 初期値でスペクトラムを更新
    spectrumDisplay.setCrossoverFrequency(static_cast<float>(crossoverSlider.getValue()));
    spectrumDisplay.setSlope(audioProcessor.getCurrentSlopeDB());

    // スペクトラムディスプレイ
    addAndMakeVisible(spectrumDisplay);

    setSize(800, 520);
}

BassSplitterAudioProcessorEditor::~BassSplitterAudioProcessorEditor()
{
    openGLContext.detach();
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

    // コントロールエリアを2列に分割
    auto controlArea = area;
    int halfWidth = controlArea.getWidth() / 2;
    
    // 左側：クロスオーバーノブ
    auto leftArea = controlArea.removeFromLeft(halfWidth);
    freqLabel.setBounds(leftArea.removeFromTop(20));
    leftArea.removeFromTop(5);
    auto knobSection = leftArea.removeFromTop(110);
    crossoverSlider.setBounds(knobSection.withSizeKeepingCentre(100, 110));
    
    // 右側：スロープ選択
    auto rightArea = controlArea;
    slopeLabel.setBounds(rightArea.removeFromTop(20));
    rightArea.removeFromTop(5);
    auto comboSection = rightArea.removeFromTop(30);
    slopeComboBox.setBounds(comboSection.withSizeKeepingCentre(120, 28));
}
