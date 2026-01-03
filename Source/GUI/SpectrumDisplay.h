#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../DSP/SpectrumAnalyzer.h"

/**
 * SpectrumDisplay - スペクトラムアナライザーの描画コンポーネント
 * クロスオーバー周波数も視覚的に表示
 */
class SpectrumDisplay : public juce::Component,
                        public juce::Timer
{
public:
    SpectrumDisplay(SpectrumAnalyzer& analyzerRef);
    ~SpectrumDisplay() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    /** クロスオーバー周波数を設定 */
    void setCrossoverFrequency(float freq);

private:
    SpectrumAnalyzer& analyzer;
    float crossoverFrequency = 200.0f;

    /** 周波数をX座標に変換（対数スケール） */
    float frequencyToX(float freq) const;

    /** X座標を周波数に変換 */
    float xToFrequency(float x) const;

    // 表示範囲
    static constexpr float minFreq = 20.0f;
    static constexpr float maxFreq = 20000.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumDisplay)
};
