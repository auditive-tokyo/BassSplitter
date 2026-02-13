#pragma once

#include "../DSP/SpectrumAnalyzer.h"
#include "EQOverlay.h"

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * SpectrumDisplay - スペクトラムアナライザーの描画コンポーネント
 * FFTスペクトラム描画 + EQOverlay（子コンポーネント）
 */
class SpectrumDisplay : public juce::Component, public juce::Timer
{
public:
    static constexpr int numBands = 6;

    SpectrumDisplay(SpectrumAnalyzer& analyzerRef);
    ~SpectrumDisplay() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    void visibilityChanged() override;

    /** EQオーバーレイへのアクセス */
    EQOverlay& getEQOverlay() { return eqOverlay; }

private:
    SpectrumAnalyzer& analyzer;
    EQOverlay eqOverlay;

    /** 周波数をX座標に変換（対数スケール） */
    float frequencyToX(float freq) const;

    // 表示範囲
    static constexpr float minFreq = 20.0f;
    static constexpr float maxFreq = 20000.0f;
    static constexpr float minDB = -48.0f;
    static constexpr float maxDB = 6.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumDisplay)
};
