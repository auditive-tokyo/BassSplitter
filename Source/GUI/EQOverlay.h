#pragma once

#include "EQCoordinateMapper.h"

#include <array>
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * EQOverlay - スペクトラム上でEQカーブを直接編集するオーバーレイ
 * ダブルクリックでHP/LPポイント作成、ドラッグで周波数調整
 * FabFilter ProQ風のインラインEQ編集
 */
class EQOverlay : public juce::Component
{
public:
    static constexpr int numBands = 6;

    /** コンストラクタ */
    EQOverlay();
    
    /** デストラクタ */
    ~EQOverlay() override = default;

    /** 描画処理 */
    void paint(juce::Graphics& g) override;

    // ---- マウス操作 ----
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

    // ---- キーボード入力 ----
    bool keyPressed(const juce::KeyPress& key) override;

    /** 各バンドのEQ周波数を設定（外部からの同期） */
    void setBandEQFrequencies(int bandIndex, float highpassFreq, float lowpassFreq);

    /** スロープを設定 */
    void setSlope(int slopeDB);

    /** バンドのバイパス状態を設定 */
    void setBandBypassed(int bandIndex, bool bypassed);

    /** フォーカスされたバンドを設定 (-1 = なし) */
    void setFocusedBand(int bandIndex);

    /** フォーカスされているバンドを取得 */
    int getFocusedBand() const { return focusedBand; }

    /** EQ周波数変更時のコールバックを設定 (bandIndex, isHighpass, newFreq) */
    void setEQFrequencyChangedCallback(std::function<void(int, bool, float)> callback)
    {
        onEQFrequencyChanged = std::move(callback);
    }

private:
    // ---- Private データメンバー ----
    std::function<void(int, bool, float)> onEQFrequencyChanged;
    // ---- Private データメンバー ----
    std::array<float, numBands> bandHighpassFreqs = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    std::array<float, numBands> bandLowpassFreqs = {20000.0f, 20000.0f, 20000.0f, 20000.0f, 20000.0f, 20000.0f};
    std::array<bool, numBands> bandBypassed = {false, true, true, true, true, false};
    int slopeDB = 24;

    // ドラッグ状態
    struct DragState
    {
        bool isDragging = false;
        int bandIndex = -1;
        bool isHighpass = true;
    };
    DragState dragState;

    // ホバー状態
    int hoveredBand = -1;
    bool hoveredIsHighpass = true;

    // ポップアップ状態（ダブルクリック時のHP/LP選択）
    struct PopupState
    {
        bool isVisible = false;
        int bandIndex = -1;
        float frequency = 0.0f; // 周波数
        float triggerX = 0.0f;  // トリガー時のマウスX座標
        float triggerY = 0.0f;  // トリガー時のマウスY座標
        float displayX = 0.0f;  // 実際の描画X座標（計算済み・固定）
        float displayY = 0.0f;  // 実際の描画Y座標（計算済み・固定）
        bool canHP = true;
        bool canLP = true;
    };
    PopupState popup;

    // フォーカス状態
    int focusedBand = -1; // -1 = フォーカスなし、0-5 = バンド番号

    // 周波数入力状態（右クリックで数値入力）
    struct FrequencyInputState
    {
        bool isActive = false;
        int bandIndex = -1;
        bool isHighpass = true;
        juce::String inputText;
    };
    FrequencyInputState freqInput;

    // 定数
    static constexpr float handleRadius = 7.0f;
    static constexpr float handleHitRadius = 14.0f;
    static constexpr float curveHitDistance = 8.0f;

    static constexpr float minFreq = 20.0f;
    static constexpr float maxFreq = 20000.0f;
    static constexpr float minDB = -48.0f;
    static constexpr float maxDB = 6.0f;

    // 座標変換ヘルパー
    [[no_unique_address]] EQCoordinateMapper coordinateMapper;

    // ---- Private メンバー関数 ----
    // 描画
    void drawFilterCurves(juce::Graphics& g);
    void drawEQHandles(juce::Graphics& g);
    void drawDragTooltip(juce::Graphics& g);
    void drawPopup(juce::Graphics& g) const;

    // EQ計算
    float getBandGain(int bandIndex, float freq) const;
    juce::Colour getBandColour(int bandIndex) const;

    // ハンドル検索
    void findNearestHandle(float mouseX, float mouseY, int& outBand, bool& outIsHighpass) const;

    /** カーブ線上のバンドを検索（ダブルクリック用） */
    int findBandOnCurve(float mouseX, float mouseY) const;

    bool cancelInputModeIfNeeded(const juce::MouseEvent& event);
    bool handlePopupClick(float mouseX, float mouseY);
    bool startDragIfHandleHit(float mouseX, float mouseY, const juce::MouseEvent& event);
    bool showPopupIfCurveClicked(float mouseX, float mouseY, const juce::MouseEvent& event);
    bool handleDoubleClickOnHandle(float mouseX, float mouseY);
    bool handleDoubleClickOnCurve(float mouseX, float mouseY);

    /** ポップアップのHPボタン領域 */
    juce::Rectangle<float> getPopupHPButtonBounds() const;
    /** ポップアップのLPボタン領域 */
    juce::Rectangle<float> getPopupLPButtonBounds() const;
    /** ポップアップの閉じるボタン領域 */
    juce::Rectangle<float> getPopupCloseBounds() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EQOverlay)
};
