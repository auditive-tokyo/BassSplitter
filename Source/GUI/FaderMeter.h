#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * FaderMeter - Abletonスタイルのフェーダー＋メーター一体型コンポーネント
 * - 背景にレベルメーター
 * - その上にフェーダーハンドル
 * - 上部にピークdB値表示
 * - ドラッグでゲイン調整
 */
class FaderMeter : public juce::Component, public juce::Timer, public juce::Label::Listener
{
public:
    FaderMeter();
    ~FaderMeter() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

    // Label::Listener
    void labelTextChanged(juce::Label* labelThatHasChanged) override;

    /** 現在のピークレベルを設定（リニア値、ステレオ） */
    void setLevel(float leftLevel, float rightLevel);

    /** 現在のピークレベルを設定（リニア値、モノラル） */
    void setLevel(float newLevel);

    /** モノモードを設定 */
    void setMono(bool mono);

    /** ピークホールドをリセット */
    void resetPeakHold();

    /** フェーダーの色を設定 */
    void setColour(juce::Colour colour);

    /** バイパス状態を設定 */
    void setBypassed(bool bypassed);

    /** 内部Sliderへのアクセス（Attachment用） */
    juce::Slider& getSlider() { return internalSlider; }

private:
    // 内部スライダー（APVTS Attachment用）
    juce::Slider internalSlider;

    // dB値入力用ラベル（クリックで編集可能）
    juce::Label dbValueLabel;

    // レベルメーター関連（左右別々）
    float currentLevelL = 0.0f; // 左チャンネル現在の表示レベル
    float currentLevelR = 0.0f; // 右チャンネル現在の表示レベル
    float targetLevelL = 0.0f;  // 左チャンネル目標レベル
    float targetLevelR = 0.0f;  // 右チャンネル目標レベル
    float peakHoldLevel = 0.0f; // ピークホールド値（L/R最大）
    float peakHoldDB = -70.0f;  // ピークホールドのdB値

    juce::Colour faderColour = juce::Colour(0xff4a90d9);
    bool isBypassed = false;
    bool isMono = false; // モノモードフラグ

    // 減衰設定
    static constexpr float attackCoeff = 0.9f;
    static constexpr float releaseCoeff = 0.93f;

    // dB範囲
    static constexpr float minDB = -70.0f;
    static constexpr float maxDB = 6.0f;

    /** リニア値をdBに変換 */
    static float linearToDB(float linear);

    /** dBを正規化値（0.0〜1.0）に変換 */
    float dbToNormalized(float db) const;

    /** Y座標をdB値に変換 */
    float yToDb(float y, float trackTop, float trackBottom) const;

    /** dB値をY座標に変換 */
    float dbToY(float db, float trackTop, float trackBottom) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FaderMeter)
};
