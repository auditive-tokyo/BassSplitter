#pragma once

#include "../DSP/SpectrumAnalyzer.h"

#include <array>
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * SpectrumDisplay - スペクトラムアナライザーの描画コンポーネント
 * クロスオーバー周波数とフィルターカーブも視覚的に表示（6バンド対応）
 */
class SpectrumDisplay : public juce::Component, public juce::Timer
{
public:
    static constexpr int numBands = 6;
    static constexpr int numCrossovers = numBands - 1;

    SpectrumDisplay(SpectrumAnalyzer& analyzerRef);
    ~SpectrumDisplay() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    void visibilityChanged() override;

    /** クロスオーバー周波数を設定（5つ） */
    void setCrossoverFrequencies(const std::array<float, numCrossovers>& freqs);

    /** スロープを設定 (12, 24, 48, 96, 192 dB/oct) */
    void setSlope(int slopeDB);

    /** バンドのバイパス状態を設定 */
    void setBandBypassed(int bandIndex, bool bypassed);

private:
    SpectrumAnalyzer& analyzer;
    std::array<float, numCrossovers> crossoverFrequencies = {80.0f, 250.0f, 1000.0f, 4000.0f, 12000.0f};
    std::array<bool, numBands> bandBypassed = {false, true, true, true, true, false};
    int slopeDB = 24;

    /** 周波数をX座標に変換（対数スケール） */
    float frequencyToX(float freq) const;

    /** X座標を周波数に変換 */
    float xToFrequency(float x) const;

    /** フィルターカーブを描画（6バンド） */
    void drawFilterCurves(juce::Graphics& g, float height);

    /** 特定バンドのゲインを計算 (dB) */
    float getBandGain(int bandIndex, float freq) const;

    /** バンドカラーを取得 */
    juce::Colour getBandColour(int bandIndex) const;

    // 表示範囲
    static constexpr float minFreq = 20.0f;
    static constexpr float maxFreq = 20000.0f;
    static constexpr float minDB = -48.0f;
    static constexpr float maxDB = 6.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumDisplay)
};
