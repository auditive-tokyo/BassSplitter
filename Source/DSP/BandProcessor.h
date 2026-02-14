#pragma once

#include <array>
#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

/**
 * BandProcessor - 単一バンドのDSP処理を担当
 * 
 * 各バンドのEQフィルタリング、モノ処理、パン、ピーク計算を実行
 */
class BandProcessor
{
public:
    // BandEQ構造体の定義（PluginProcessorから移動）
    struct BandEQ
    {
        std::array<juce::dsp::LinkwitzRileyFilter<float>, 8> highpass;
        std::array<juce::dsp::LinkwitzRileyFilter<float>, 8> lowpass;
    };

    // BandParameters構造体の定義（PluginProcessorから移動）
    struct BandParameters
    {
        std::array<bool, 6> bypassed;
        std::array<bool, 6> soloed;
        std::array<bool, 6> mono;
        std::array<float, 6> pans;
        std::array<float, 6> gains;
        bool anySolo;
    };

    // 単一バンド処理用のコンテキスト構造体
    struct BandProcessingContext
    {
        int band;
        int numStages;
        int numChannels;
        int numSamples;
        juce::AudioBuffer<float>& bandBuffer;
        BandEQ& bandEQ;
        std::atomic<float>& peakLevelL;
        std::atomic<float>& peakLevelR;
    };

    BandProcessor() = default;

    /**
     * 単一バンドの完全な処理を実行
     */
    void processSingleBand(const BandParameters& params, const BandProcessingContext& context) const;

private:
    void applyEQFilters(juce::AudioBuffer<float>& bandBuffer, BandEQ& bandEQ, int numStages) const;
    void applyMonoProcessing(juce::AudioBuffer<float>& bandBuffer, int numSamples) const;
    void calculateMonoPeaks(const juce::AudioBuffer<float>& bandBuffer, int numSamples, float& peakL, float& peakR) const;
    void applyPanning(juce::AudioBuffer<float>& bandBuffer, float panValue, int numChannels, int numSamples) const;
    void calculateStereoPeaks(const juce::AudioBuffer<float>& bandBuffer, int numChannels, int numSamples, 
                             float& peakL, float& peakR) const;
    void applyGainAndStorePeaks(float gain, float peakL, float peakR, 
                               std::atomic<float>& peakLevelL, std::atomic<float>& peakLevelR) const;
};
