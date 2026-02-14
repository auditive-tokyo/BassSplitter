#pragma once

/**
 * EQCoordinateMapper - EQ表示の座標変換を担当
 * 周波数⇔X座標、dB⇔Y座標の変換を提供
 */
class EQCoordinateMapper
{
public:
    /** コンストラクタ */
    EQCoordinateMapper() = default;

    /** 周波数をX座標に変換（対数スケール） */
    float frequencyToX(float freq, int width) const;

    /** X座標を周波数に変換（対数スケール） */
    float xToFrequency(float x, int width) const;

    /** dBをY座標に変換（線形スケール） */
    float dbToY(float db, int height) const;

private:
    static constexpr float minFreq = 20.0f;
    static constexpr float maxFreq = 20000.0f;
    static constexpr float minDB = -48.0f;
    static constexpr float maxDB = 6.0f;
};
