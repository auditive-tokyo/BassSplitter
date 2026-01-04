#include "PluginEditor.h"

#include "PluginProcessor.h"

BassSplitterAudioProcessorEditor::BassSplitterAudioProcessorEditor(BassSplitterAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), spectrumDisplay(p.getSpectrumAnalyzer())
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

    // スロープラベル
    slopeLabel.setText("Slope", juce::dontSendNotification);
    slopeLabel.setFont(juce::Font(12.0f));
    slopeLabel.setJustificationType(juce::Justification::centred);
    slopeLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(slopeLabel);

    // スロープコンボボックス
    slopeComboBox.addItem("12 dB/oct", 1);
    slopeComboBox.addItem("24 dB/oct", 2);
    slopeComboBox.addItem("48 dB/oct", 3);
    slopeComboBox.addItem("96 dB/oct", 4);
    slopeComboBox.addItem("192 dB/oct", 5);
    slopeComboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff0d0d1a));
    slopeComboBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xff4a90d9));
    slopeComboBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff333344));
    slopeComboBox.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xff4a90d9));
    addAndMakeVisible(slopeComboBox);
    slopeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), "slope", slopeComboBox);

    // クロスオーバーコントロール（5つ、バンド間に配置）
    for (int i = 0; i < BassSplitterAudioProcessor::numCrossovers; ++i)
    {
        auto& control = crossoverControls[static_cast<size_t>(i)];

        // スライダー（縦向きロータリー）
        control.slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        control.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 55, 16);
        control.slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff4a90d9));
        control.slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff333344));
        control.slider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
        control.slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xff4a90d9));
        control.slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff0d0d1a));
        control.slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff333344));
        control.slider.setTextValueSuffix(" Hz");
        addAndMakeVisible(control.slider);

        // ラベル（どのバンド間かを表示）
        control.label.setText(juce::String(i + 1) + "-" + juce::String(i + 2), juce::dontSendNotification);
        control.label.setFont(juce::Font(10.0f));
        control.label.setJustificationType(juce::Justification::centred);
        control.label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        addAndMakeVisible(control.label);

        control.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), "crossover" + juce::String(i + 1), control.slider);
    }

    // 6バンドのコントロールをセットアップ
    for (int i = 0; i < BassSplitterAudioProcessor::numBands; ++i)
    {
        setupBandControls(i);
    }

    // スペクトラムディスプレイ
    addAndMakeVisible(spectrumDisplay);

    // ピークリセットボタン
    resetPeaksButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff333344));
    resetPeaksButton.setColour(juce::TextButton::textColourOffId, juce::Colours::lightgrey);
    resetPeaksButton.onClick = [this]()
    {
        for (auto& controls : bandControls)
            controls.faderMeter.resetPeakHold();
    };
    addAndMakeVisible(resetPeaksButton);

    // タイマー開始（スペクトラムディスプレイとレベルメーター更新用）
    startTimerHz(60);

    // 初期状態を設定
    updateSpectrumDisplay();

    setSize(1000, 750);
}

void BassSplitterAudioProcessorEditor::setupBandControls(int bandIndex)
{
    auto& controls = bandControls[static_cast<size_t>(bandIndex)];
    juce::String bandId = "band" + juce::String(bandIndex + 1);

    // バンドカラー（グラデーション：左は深い青、右はライトグリーン）
    // Band 1 (index 0) = Deep Blue (hue 0.65), Band 6 (index 5) = Light Green (hue 0.35)
    float hue = 0.65f - (static_cast<float>(bandIndex) / (BassSplitterAudioProcessor::numBands - 1)) * 0.30f;
    juce::Colour bandColour = juce::Colour::fromHSV(hue, 0.75f, 0.95f, 1.0f);

    // 名前ラベル
    controls.nameLabel.setText(audioProcessor.getBandName(bandIndex), juce::dontSendNotification);
    controls.nameLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    controls.nameLabel.setJustificationType(juce::Justification::centred);
    controls.nameLabel.setColour(juce::Label::textColourId, bandColour);
    controls.nameLabel.setEditable(true);
    controls.nameLabel.onTextChange = [this, bandIndex, &controls]()
    { audioProcessor.setBandName(bandIndex, controls.nameLabel.getText()); };
    addAndMakeVisible(controls.nameLabel);

    // バイパスボタン
    controls.bypassButton.setClickingTogglesState(true);
    controls.bypassButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff333344));
    controls.bypassButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff666666));
    controls.bypassButton.setColour(juce::TextButton::textColourOffId, juce::Colours::lightgrey);
    controls.bypassButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    addAndMakeVisible(controls.bypassButton);
    controls.bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), bandId + "Bypass", controls.bypassButton);

    // ソロボタン
    controls.soloButton.setClickingTogglesState(true);
    controls.soloButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff333344));
    controls.soloButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffffcc00));
    controls.soloButton.setColour(juce::TextButton::textColourOffId, juce::Colours::lightgrey);
    controls.soloButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    addAndMakeVisible(controls.soloButton);
    controls.soloAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), bandId + "Solo", controls.soloButton);

    // フェーダーメーター（一体型）
    controls.faderMeter.setColour(bandColour);
    addAndMakeVisible(controls.faderMeter);
    controls.gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), bandId + "Gain", controls.faderMeter.getSlider());
}

BassSplitterAudioProcessorEditor::~BassSplitterAudioProcessorEditor()
{
    stopTimer();
    openGLContext.detach();
}

void BassSplitterAudioProcessorEditor::timerCallback()
{
    updateSpectrumDisplay();
    updateLevelMeters();
}

void BassSplitterAudioProcessorEditor::updateSpectrumDisplay()
{
    // クロスオーバー周波数を更新
    std::array<float, BassSplitterAudioProcessor::numCrossovers> freqs;
    for (int i = 0; i < BassSplitterAudioProcessor::numCrossovers; ++i)
    {
        auto* param = audioProcessor.getAPVTS().getRawParameterValue("crossover" + juce::String(i + 1));
        freqs[static_cast<size_t>(i)] = param->load();
    }
    spectrumDisplay.setCrossoverFrequencies(freqs);

    // スロープを更新
    auto* slopeParam = audioProcessor.getAPVTS().getRawParameterValue("slope");
    int slopeIndex = static_cast<int>(slopeParam->load());
    static const int slopeValues[] = {12, 24, 48, 96, 192};
    spectrumDisplay.setSlope(slopeValues[slopeIndex]);

    // 各バンドのバイパス状態を更新
    for (int i = 0; i < BassSplitterAudioProcessor::numBands; ++i)
    {
        auto* bypassParam = audioProcessor.getAPVTS().getRawParameterValue("band" + juce::String(i + 1) + "Bypass");
        spectrumDisplay.setBandBypassed(i, bypassParam->load() > 0.5f);
    }
}

void BassSplitterAudioProcessorEditor::updateLevelMeters()
{
    for (int i = 0; i < BassSplitterAudioProcessor::numBands; ++i)
    {
        auto& controls = bandControls[static_cast<size_t>(i)];

        // バイパス状態を確認
        auto* bypassParam = audioProcessor.getAPVTS().getRawParameterValue("band" + juce::String(i + 1) + "Bypass");
        bool bypassed = bypassParam->load() > 0.5f;
        controls.faderMeter.setBypassed(bypassed);

        if (!bypassed)
        {
            // ピークレベルを取得してメーターに設定
            float peakLevel = audioProcessor.getBandPeakLevel(i);
            controls.faderMeter.setLevel(peakLevel);
        }
    }
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

    // タイトル
    titleLabel.setBounds(area.removeFromTop(35));
    area.removeFromTop(5);

    // スペクトラムディスプレイ
    spectrumDisplay.setBounds(area.removeFromTop(200));
    area.removeFromTop(10);

    // スロープ選択とピークリセット（上部に配置）
    auto slopeArea = area.removeFromTop(30);
    slopeLabel.setBounds(slopeArea.removeFromLeft(50));
    slopeComboBox.setBounds(slopeArea.removeFromLeft(120).reduced(5, 2));
    slopeArea.removeFromLeft(20); // スペース
    resetPeaksButton.setBounds(slopeArea.removeFromLeft(80).reduced(5, 2));
    area.removeFromTop(15);

    // バンドとクロスオーバーを交互に配置
    // 全体幅を17分割（6バンド x 2 + 5クロスオーバー）
    int totalWidth = area.getWidth();
    int bandWidth = totalWidth * 2 / 17;  // バンドは少し広め
    int crossoverWidth = totalWidth / 17; // クロスオーバーは狭め

    auto controlsArea = area;

    for (int i = 0; i < BassSplitterAudioProcessor::numBands; ++i)
    {
        // バンドコントロール
        auto bandArea = controlsArea.removeFromLeft(bandWidth);
        auto& controls = bandControls[static_cast<size_t>(i)];

        controls.nameLabel.setBounds(bandArea.removeFromTop(20));
        bandArea.removeFromTop(5);

        auto buttonArea = bandArea.removeFromTop(25);
        int buttonWidth = buttonArea.getWidth() / 2;
        controls.bypassButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(3, 0));
        controls.soloButton.setBounds(buttonArea.reduced(3, 0));

        bandArea.removeFromTop(5);

        // FaderMeter（フェーダーとレベルメーター一体型）
        controls.faderMeter.setBounds(bandArea.reduced(4, 0));

        // クロスオーバーコントロール（バンド間に配置、最後のバンドの後は不要）
        if (i < BassSplitterAudioProcessor::numCrossovers)
        {
            auto crossoverArea = controlsArea.removeFromLeft(crossoverWidth);
            auto& crossover = crossoverControls[static_cast<size_t>(i)];

            crossover.label.setBounds(crossoverArea.removeFromTop(15));
            crossover.slider.setBounds(crossoverArea.reduced(2, 0));
        }
    }
}
