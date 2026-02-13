#include "EQPanel.h"

EQPanel::EQPanel(BassSplitterAudioProcessor& processor)
    : audioProcessor(processor)
{
    setupControls();
}

EQPanel::~EQPanel() {}

void EQPanel::setupControls()
{
    // 6バンド分のコントロール設定
    for (int i = 0; i < BassSplitterAudioProcessor::numBands; ++i)
    {
        juce::String bandId = "band" + juce::String(i + 1);
        juce::String bandName = "Band " + juce::String(i + 1);

        const auto idx = static_cast<size_t>(i);

        // バンドラベル
        bandLabels[idx].setText(bandName, juce::dontSendNotification);
        bandLabels[idx].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(bandLabels[idx]);

        // Highpass スライダー
        highpassSliders[idx].setSliderStyle(juce::Slider::LinearHorizontal);
        highpassSliders[idx].setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
        highpassSliders[idx].setRange(0.0f, 20000.0f, 1.0f);
        highpassSliders[idx].setSkewFactorFromMidPoint(2000.0f);
        addAndMakeVisible(highpassSliders[idx]);

        highpassAttachments[idx] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), bandId + "HighpassFreq", highpassSliders[idx]);

        highpassLabels[idx].setText("High", juce::dontSendNotification);
        highpassLabels[idx].setJustificationType(juce::Justification::right);
        addAndMakeVisible(highpassLabels[idx]);

        // Lowpass スライダー
        lowpassSliders[idx].setSliderStyle(juce::Slider::LinearHorizontal);
        lowpassSliders[idx].setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
        lowpassSliders[idx].setRange(0.0f, 20000.0f, 1.0f);
        lowpassSliders[idx].setSkewFactorFromMidPoint(2000.0f);
        addAndMakeVisible(lowpassSliders[idx]);

        lowpassAttachments[idx] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), bandId + "LowpassFreq", lowpassSliders[idx]);

        lowpassLabels[idx].setText("Low", juce::dontSendNotification);
        lowpassLabels[idx].setJustificationType(juce::Justification::right);
        addAndMakeVisible(lowpassLabels[idx]);

        // バンド選択ボタン
        bandButtons[idx].setButtonText(bandName);
        bandButtons[idx].setRadioGroupId(1001);
        bandButtons[idx].setClickingTogglesState(true);
        addAndMakeVisible(bandButtons[idx]);

        bandButtons[idx].onClick = [this, i]()
        {
            setSelectedBand(i);
        };

        // Band 1 をデフォルト選択
        if (i == 0)
        {
            bandButtons[idx].setToggleState(true, juce::dontSendNotification);
        }
    }

    updateBandColor();
}

void EQPanel::paint(juce::Graphics& g)
{
    // 背景
    g.fillAll(juce::Colour(0xff1a1a2e).withAlpha(0.95f));

    // ボーダー
    g.setColour(juce::Colour(0xff4a90d9));
    g.drawRect(getLocalBounds(), 2);
}

void EQPanel::resized()
{
    auto area = getLocalBounds().reduced(10);

    // バンド選択ボタン（上部）
    auto buttonArea = area.removeFromTop(30);
    int buttonWidth = getWidth() / BassSplitterAudioProcessor::numBands;
    for (int i = 0; i < BassSplitterAudioProcessor::numBands; ++i)
    {
        bandButtons[static_cast<size_t>(i)].setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2, 2));
    }

    area.removeFromTop(10);

    // 選択中バンドのコントロール表示
    const int controlHeight = 25;
    const int labelWidth = 50;
    const int spacing = 10;

    // Highpass
    const auto selectedIdx = static_cast<size_t>(selectedBandIndex);
    auto highpassArea = area.removeFromTop(controlHeight + spacing);
    highpassLabels[selectedIdx].setBounds(highpassArea.removeFromLeft(labelWidth));
    highpassSliders[selectedIdx].setBounds(highpassArea);

    // Lowpass
    auto lowpassArea = area.removeFromTop(controlHeight);
    lowpassLabels[selectedIdx].setBounds(lowpassArea.removeFromLeft(labelWidth));
    lowpassSliders[selectedIdx].setBounds(lowpassArea);

    // 非選択バンドコントロールは隠す
    for (int i = 0; i < BassSplitterAudioProcessor::numBands; ++i)
    {
        const auto idx = static_cast<size_t>(i);
        highpassSliders[idx].setVisible(i == selectedBandIndex);
        lowpassSliders[idx].setVisible(i == selectedBandIndex);
        highpassLabels[idx].setVisible(i == selectedBandIndex);
        lowpassLabels[idx].setVisible(i == selectedBandIndex);
    }
}

void EQPanel::setSelectedBand(int bandIndex)
{
    if (bandIndex >= 0 && bandIndex < BassSplitterAudioProcessor::numBands)
    {
        selectedBandIndex = bandIndex;
        bandButtons[static_cast<size_t>(bandIndex)].setToggleState(true, juce::dontSendNotification);
        updateBandColor();
        resized();
    }
}

void EQPanel::updateBandColor()
{
    // バンド色（SpectrumDisplay と同じ色計算）
    float hue = 0.65f - (static_cast<float>(selectedBandIndex) / (BassSplitterAudioProcessor::numBands - 1)) * 0.30f;
    juce::Colour bandColour = juce::Colour::fromHSV(hue, 0.75f, 0.95f, 1.0f);

    // 選択中のボタンを色付け
    for (int i = 0; i < BassSplitterAudioProcessor::numBands; ++i)
    {
        const auto idx = static_cast<size_t>(i);
        if (i == selectedBandIndex)
        {
            bandButtons[idx].setColour(juce::TextButton::buttonColourId, bandColour);
            bandButtons[idx].setColour(juce::TextButton::textColourOnId, juce::Colours::black);
        }
        else
        {
            bandButtons[idx].setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a4e));
            bandButtons[idx].setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        }
    }

    // スライダーをバンド色に
    const auto selectedIdx2 = static_cast<size_t>(selectedBandIndex);
    highpassSliders[selectedIdx2].setColour(juce::Slider::thumbColourId, bandColour);
    lowpassSliders[selectedIdx2].setColour(juce::Slider::thumbColourId, bandColour);
}
