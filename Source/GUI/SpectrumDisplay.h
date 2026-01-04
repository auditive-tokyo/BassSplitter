#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../DSP/SpectrumAnalyzer.h"

/**
 * SpectrumDisplay - スペクトラムアナライザーの描画コンポーネント
 * クロスオーバー周波数とフィルターカーブも視覚的に表示
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

    /** スロープを設定 (12, 24, 48 dB/oct) */
    void setSlope(int slopeDB);

private:
    SpectrumAnalyzer& analyzer;
    float crossoverFrequency = 200.0f;
    int slopeDB = 24;  // デフォルト: 24dB/oct

    /** 周波数をX座標に変換（対数スケール） */
    float frequencyToX(float freq) const;

    /** X座標を周波数に変換 */
    float xToFrequency(float x) const;

    /** フィルターカーブを描画 */
    void drawFilterCurve(juce::Graphics& g, float height);

    /** ローパスフィルターのゲインを計算 (dB) */
    float getLowpassGain(float freq) const;

    /** ハイパスフィルターのゲインを計算 (dB) */
    float getHighpassGain(float freq) const;

    // 表示範囲
    static constexpr float minFreq = 20.0f;
    static constexpr float maxFreq = 20000.0f;
    static constexpr float minDB = -48.0f;  // 表示下限
    static constexpr float maxDB = 6.0f;    // 表示上限

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumDisplay)
};
