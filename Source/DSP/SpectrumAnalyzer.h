#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <span>

/**
 * SpectrumAnalyzer - FFTを使用してオーディオ信号の周波数スペクトラムを解析
 */
class SpectrumAnalyzer
{
public:
    // FFTサイズ（2048 = 良いバランス）
    static constexpr int fftOrder = 11;  // 2^11 = 2048
    static constexpr int fftSize = 1 << fftOrder;  // 2048

    SpectrumAnalyzer();

    /** サンプルレートを設定 */
    void setSampleRate(double newSampleRate);

    /** オーディオサンプルをプッシュ（processBlockから呼ぶ） */
    void pushSamples(std::span<const float> samples);

    /** 次のFFTブロックが準備できているか */
    bool isNextBlockReady() const { return nextFFTBlockReady; }

    /** FFTを実行してスペクトラムデータを取得 */
    void processFFT();

    /** スペクトラムデータを取得（0.0〜1.0の正規化された値） */
    const std::array<float, fftSize / 2>& getSpectrumData() const { return spectrumData; }

    /** サンプルレートを取得 */
    double getSampleRate() const { return sampleRate; }

private:
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;

    std::array<float, fftSize> fftData;
    std::array<float, fftSize * 2> fifo;
    std::array<float, fftSize / 2> spectrumData;

    int fifoIndex = 0;
    bool nextFFTBlockReady = false;
    double sampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzer)
};
