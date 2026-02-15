#include "PluginEditor.h"

#include "GUI/StyleHelper.h"
#include "PluginProcessor.h"

BassSplitterAudioProcessorEditor::BassSplitterAudioProcessorEditor(BassSplitterAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), spectrumDisplay(p.getSpectrumAnalyzer())
{
    // GPU描画を有効化
    openGLContext.setComponentPaintingEnabled(true);
    openGLContext.attachTo(*this);

    // タイトルラベル
    titleLabel.setText("BassSplitter", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);

    // スロープラベル
    slopeLabel.setText("Slope", juce::dontSendNotification);
    slopeLabel.setFont(juce::FontOptions(12.0f));
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

    // 6バンドのコントロールをセットアップ
    for (int i = 0; i < BassSplitterAudioProcessor::numBands; ++i)
    {
        setupBandControls(i);
    }

    // スペクトラムディスプレイ（EQOverlay内蔵）
    addAndMakeVisible(spectrumDisplay);

    // EQOverlayのコールバック→APVTSパラメータ更新
    spectrumDisplay.getEQOverlay().setEQFrequencyChangedCallback(
        [this](int bandIndex, bool isHighpass, float newFreq)
        {
            juce::String bandId = "band" + juce::String(bandIndex + 1);
            juce::String paramId = bandId + (isHighpass ? "HighpassFreq" : "LowpassFreq");
            if (auto* param = audioProcessor.getAPVTS().getParameter(paramId))
            {
                float normalized = param->convertTo0to1(newFreq);
                param->setValueNotifyingHost(normalized);
            }
        });

    // ピークリセットボタン
    resetPeaksButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff333344));
    resetPeaksButton.setColour(juce::TextButton::textColourOffId, juce::Colours::lightgrey);
    resetPeaksButton.onClick = [this]() // NOSONAR - member access requires this capture.
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

    float hue = 0.65f - (static_cast<float>(bandIndex) / (BassSplitterAudioProcessor::numBands - 1)) * 0.30f;
    juce::Colour bandColour = juce::Colour::fromHSV(hue, 0.75f, 0.95f, 1.0f);

    // チャンネル名ラベル（共通スタイル適用）
    controls.nameLabel.setText(audioProcessor.getBandName(bandIndex), juce::dontSendNotification);
    StyleHelper::applyEditableLabelStyle(controls.nameLabel);
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

    // EQボタン（スペクトラム上でのバンドフォーカス）
    controls.eqButton.setClickingTogglesState(false);
    controls.eqButton.setColour(juce::TextButton::buttonColourId, bandColour.withAlpha(0.7f));
    controls.eqButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(controls.eqButton);
    controls.eqButton.onClick = [this, bandIndex]() { spectrumDisplay.getEQOverlay().setFocusedBand(bandIndex); };

    // ソロボタン
    controls.soloButton.setClickingTogglesState(true);
    controls.soloButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff333344));
    controls.soloButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffffcc00));
    controls.soloButton.setColour(juce::TextButton::textColourOffId, juce::Colours::lightgrey);
    controls.soloButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    addAndMakeVisible(controls.soloButton);
    controls.soloAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), bandId + "Solo", controls.soloButton);

    // モノボタン
    controls.monoButton.setClickingTogglesState(true);
    controls.monoButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff333344));
    controls.monoButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff66aaff));
    controls.monoButton.setColour(juce::TextButton::textColourOffId, juce::Colours::lightgrey);
    controls.monoButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    addAndMakeVisible(controls.monoButton);
    controls.monoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), bandId + "Mono", controls.monoButton);

    // FXチェーンボタン
    controls.fxButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a3e));
    controls.fxButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8ec8ff));
    addAndMakeVisible(controls.fxButton);
    controls.fxButton.onClick = [this, bandIndex]()
    {
        auto idx = static_cast<size_t>(bandIndex);
        if (!fxWindows[idx])
        {
            fxWindows[idx] = std::make_unique<FXChainWindow>(
                audioProcessor.getBandName(bandIndex), bandIndex);
        }
        fxWindows[idx]->setVisible(true);
        fxWindows[idx]->toFront(true);
    };

    // パンコントロール（ノブ）
    controls.panControl.configure(bandColour);
    controls.panControl.attachToParameter(audioProcessor.getAPVTS(), bandId + "Pan");
    addAndMakeVisible(controls.panControl);

    // フェーダーメーター（一体型）
    controls.faderMeter.setFaderColour(bandColour);
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
    auto& eqOverlay = spectrumDisplay.getEQOverlay();

    // スロープを更新
    const auto* slopeParam = audioProcessor.getAPVTS().getRawParameterValue("slope");
    auto slopeIndex = static_cast<int>(slopeParam->load());
    static const std::array<int, 5> slopeValues = {12, 24, 48, 96, 192};
    eqOverlay.setSlope(slopeValues[static_cast<size_t>(slopeIndex)]);

    // 各バンドのバイパス状態とEQ周波数を更新
    for (int i = 0; i < BassSplitterAudioProcessor::numBands; ++i)
    {
        juce::String bandId = "band" + juce::String(i + 1);
        const auto* bypassParam = audioProcessor.getAPVTS().getRawParameterValue(bandId + "Bypass");
        eqOverlay.setBandBypassed(i, bypassParam->load() > 0.5f);

        float hpFreq = audioProcessor.getAPVTS().getRawParameterValue(bandId + "HighpassFreq")->load();
        float lpFreq = audioProcessor.getAPVTS().getRawParameterValue(bandId + "LowpassFreq")->load();
        eqOverlay.setBandEQFrequencies(i, hpFreq, lpFreq);
    }
}

void BassSplitterAudioProcessorEditor::updateLevelMeters()
{
    for (int i = 0; i < BassSplitterAudioProcessor::numBands; ++i)
    {
        auto& controls = bandControls[static_cast<size_t>(i)];
        juce::String bandId = "band" + juce::String(i + 1);

        const auto* bypassParam = audioProcessor.getAPVTS().getRawParameterValue(bandId + "Bypass");
        bool bypassed = bypassParam->load() > 0.5f;
        controls.faderMeter.setBypassed(bypassed);

        const auto* monoParam = audioProcessor.getAPVTS().getRawParameterValue(bandId + "Mono");
        bool mono = monoParam->load() > 0.5f;
        controls.faderMeter.setMono(mono);

        if (!bypassed)
        {
            float peakL;
            float peakR;
            audioProcessor.getBandPeakLevelStereo(i, peakL, peakR);
            controls.faderMeter.setLevel(peakL, peakR);
        }
    }
}

void BassSplitterAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a2e));

    g.setColour(juce::Colour(0xff4a90d9));
    g.drawRect(getLocalBounds().reduced(10), 2);
}

void BassSplitterAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);

    // タイトル
    titleLabel.setBounds(area.removeFromTop(35));
    area.removeFromTop(5);

    // スペクトラムディスプレイ（EQOverlay込み）
    auto spectrumArea = area.removeFromTop(200);
    spectrumDisplay.setBounds(spectrumArea);

    area.removeFromTop(10);

    // スロープ選択とピークリセット
    auto slopeArea = area.removeFromTop(30);
    slopeLabel.setBounds(slopeArea.removeFromLeft(50));
    slopeComboBox.setBounds(slopeArea.removeFromLeft(120).reduced(5, 2));
    slopeArea.removeFromLeft(20);
    resetPeaksButton.setBounds(slopeArea.removeFromLeft(80).reduced(5, 2));
    area.removeFromTop(15);

    // バンドを均等に配置
    int totalWidth = area.getWidth();
    int bandWidth = totalWidth / BassSplitterAudioProcessor::numBands;

    auto controlsArea = area;

    for (int i = 0; i < BassSplitterAudioProcessor::numBands; ++i)
    {
        auto bandArea = controlsArea.removeFromLeft(bandWidth);
        auto& controls = bandControls[static_cast<size_t>(i)];

        controls.nameLabel.setBounds(bandArea.removeFromTop(20));
        bandArea.removeFromTop(5);

        // パンコントロール（ノブ）
        controls.panControl.setBounds(bandArea.removeFromTop(95).reduced(2, 0));
        bandArea.removeFromTop(3);

        // ボタン用のエリアを下から確保（縦並び：FX, EQ, Mono, Solo, Bypass）
        auto buttonArea = bandArea.removeFromBottom(110); // 5ボタン × 22px
        controls.fxButton.setBounds(buttonArea.removeFromTop(22).reduced(4, 1));
        controls.eqButton.setBounds(buttonArea.removeFromTop(22).reduced(4, 1));
        controls.monoButton.setBounds(buttonArea.removeFromTop(22).reduced(4, 1));
        controls.soloButton.setBounds(buttonArea.removeFromTop(22).reduced(4, 1));
        controls.bypassButton.setBounds(buttonArea.removeFromTop(22).reduced(4, 1));

        // FaderMeter（フェーダーとレベルメーター一体型）
        controls.faderMeter.setBounds(bandArea.reduced(4, 0));
    }
}
