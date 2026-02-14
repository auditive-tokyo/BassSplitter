#pragma once

#include "BandProcessor.h"

#include <array>
#include <juce_audio_processors/juce_audio_processors.h>

/**
 * ProcessBlockCoordinator - processBlock全体の調整を担当
 * 
 * スロープ取得、フィルター周波数設定、パラメータロード、
 * バッファ準備、ピーク計算、出力ミックスを統括管理
 */
class ProcessBlockCoordinator
{
public:
    ProcessBlockCoordinator() = default;

    // BandProcessor用の型定義
    using BandParameters = BandProcessor::BandParameters;

    /**
     * スロープインデックスをフィルターステージ数に変換
     */
    static int getNumFilterStages(int slopeIndex);

    /**
     * 各バンドのEQフィルター周波数設定を更新
     */
    void updateEQFilterSettings(const juce::AudioProcessorValueTreeState& apvts,
                               std::array<BandProcessor::BandEQ, 6>& bandEQFilters) const;

    /**
     * 全バンドのパラメータをロード
     */
    BandParameters loadBandParameters(const juce::AudioProcessorValueTreeState& apvts) const;

    /**
     * 各バンドのバッファを準備
     */
    void prepareBandBuffers(const juce::AudioBuffer<float>& buffer,
                           std::array<juce::AudioBuffer<float>, 6>& bandBuffers,
                           int numChannels,
                           int numSamples) const;

    /**
     * バイパス中のバンドのピークをゼロに
     */
    void clearBypassedBandPeaks(const BandParameters& params,
                               std::array<std::atomic<float>, 6>& bandPeakLevelsL,
                               std::array<std::atomic<float>, 6>& bandPeakLevelsR) const;

    /**
     * バンドバッファを出力に混合
     */
    void mixBandsToOutput(juce::AudioBuffer<float>& buffer,
                         const BandParameters& params,
                         const std::array<juce::AudioBuffer<float>, 6>& bandBuffers,
                         int numChannels,
                         int numSamples) const;
};
