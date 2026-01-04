#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * LevelMeter - 縦型レベルメーター
 * - ピークメーター（スムーズな減衰）
 * - ピークホールド（最大値を保持、より高いピークで更新）
 * - Abletonスタイル
 */
class LevelMeter : public juce::Component,
                   public juce::Timer
{
public:
    LevelMeter();
    ~LevelMeter() override;

    void paint(juce::Graphics& g) override;
    void timerCallback() override;

    /** 現在のピークレベルを設定（0.0〜1.0のリニア値、または負のdB値） */
    void setLevel(float newLevel);

    /** ピークホールドをリセット */
    void resetPeakHold();

    /** メーターの色を設定 */
    void setMeterColour(juce::Colour colour);

    /** バイパス状態を設定（バイパス時はメーターをグレーアウト） */
    void setBypassed(bool bypassed);

private:
    float currentLevel = 0.0f;      // 現在の表示レベル（減衰後）
    float targetLevel = 0.0f;       // 目標レベル（入力値）
    float peakHoldLevel = 0.0f;     // ピークホールド値
    float peakHoldDB = -60.0f;      // ピークホールドのdB値（表示用）

    juce::Colour meterColour = juce::Colour(0xff4a90d9);
    bool isBypassed = false;

    // 減衰設定
    static constexpr float attackCoeff = 0.95f;   // アタック係数（高速）
    static constexpr float releaseCoeff = 0.92f;  // リリース係数（減衰速度）

    // dB範囲
    static constexpr float minDB = -60.0f;
    static constexpr float maxDB = 6.0f;

    /** リニア値をdBに変換 */
    static float linearToDB(float linear);

    /** dBを0.0〜1.0の正規化値に変換 */
    float dbToNormalized(float db) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
};
