#include "SpectrumAnalyzer.h"

SpectrumAnalyzer::SpectrumAnalyzer()
    : fft(fftOrder),
      window(fftSize, juce::dsp::WindowingFunction<float>::hann)
{
    // 配列を初期化
    fftData.fill(0.0f);
    fifo.fill(0.0f);
    spectrumData.fill(0.0f);
}

void SpectrumAnalyzer::setSampleRate(double newSampleRate)
{
    sampleRate = newSampleRate;
}

void SpectrumAnalyzer::pushSamples(const float* samples, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        // モノラルにミックス（ステレオの場合は左チャンネルを使用）
        float sample = samples[i];

        if (fifoIndex < fftSize)
        {
            fifo[static_cast<size_t>(fifoIndex)] = sample;
            ++fifoIndex;
        }

        if (fifoIndex == fftSize)
        {
            nextFFTBlockReady = true;
        }
    }
}

void SpectrumAnalyzer::processFFT()
{
    if (!nextFFTBlockReady)
        return;

    // FIFOからFFTデータにコピー
    for (size_t i = 0; i < fftSize; ++i)
        fftData[i] = fifo[i];

    // ウィンドウ関数を適用
    window.multiplyWithWindowingTable(fftData.data(), fftSize);

    // FFTを実行
    fft.performFrequencyOnlyForwardTransform(fftData.data());

    // 結果を正規化してスペクトラムデータに保存
    constexpr float minDb = -100.0f;
    constexpr float maxDb = 0.0f;

    for (size_t i = 0; i < fftSize / 2; ++i)
    {
        // 振幅をデシベルに変換
        float magnitude = fftData[i];
        float db = juce::Decibels::gainToDecibels(magnitude, minDb);

        // 0.0〜1.0に正規化
        float normalized = juce::jmap(db, minDb, maxDb, 0.0f, 1.0f);
        
        // スムージング（前の値と混合）
        spectrumData[i] = spectrumData[i] * 0.7f + normalized * 0.3f;
    }

    // FIFOをリセット
    fifoIndex = 0;
    nextFFTBlockReady = false;
}
