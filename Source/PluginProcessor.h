#pragma once

#include "DSP/SpectrumAnalyzer.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

class BassSplitterAudioProcessor : public juce::AudioProcessor
{
public:
    // バンド数
    static constexpr int numBands = 6;
    static constexpr int numCrossovers = numBands - 1; // 5つのクロスオーバーポイント

    BassSplitterAudioProcessor();
    ~BassSplitterAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // パラメータ管理（公開）
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // スペクトラムアナライザーへのアクセス
    SpectrumAnalyzer& getSpectrumAnalyzer() { return spectrumAnalyzer; }

    // 現在のスロープを取得（dB/oct）
    int getCurrentSlopeDB() const;

    // クロスオーバー周波数を取得
    float getCrossoverFrequency(int index) const;

    // バンド名を取得/設定（ValueTreeに保存）
    juce::String getBandName(int bandIndex) const;
    void setBandName(int bandIndex, const juce::String& name);

    /** 各バンドのピークレベルを取得（リニア値 0.0〜1.0+） */
    float getBandPeakLevel(int bandIndex) const;

private:
    // パラメータレイアウト作成
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // パラメータ管理
    juce::AudioProcessorValueTreeState apvts;

    // 各クロスオーバーポイント用のLinkwitz-Rileyフィルター
    // 各クロスオーバーに対してローパス/ハイパスのペア（最大8段）
    struct CrossoverFilters
    {
        std::array<juce::dsp::LinkwitzRileyFilter<float>, 8> lowpass;
        std::array<juce::dsp::LinkwitzRileyFilter<float>, 8> highpass;
    };
    std::array<CrossoverFilters, numCrossovers> crossoverFilters;

    // 各バンドの出力バッファ
    std::array<juce::AudioBuffer<float>, numBands> bandBuffers;

    // スペクトラムアナライザー
    SpectrumAnalyzer spectrumAnalyzer;

    // サンプルレート保存
    double currentSampleRate = 44100.0;

    // バンド名（ValueTreeに保存）
    std::array<juce::String, numBands> bandNames = {"Band 1", "Band 2", "Band 3", "Band 4", "Band 5", "Band 6"};

    // 各バンドのピークレベル（アトミック、GUIから読み取り）
    std::array<std::atomic<float>, numBands> bandPeakLevels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassSplitterAudioProcessor)
};
