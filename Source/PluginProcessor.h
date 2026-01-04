#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/SpectrumAnalyzer.h"

class BassSplitterAudioProcessor : public juce::AudioProcessor
{
public:
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

private:
    // パラメータレイアウト作成
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // パラメータ管理
    juce::AudioProcessorValueTreeState apvts;

    // Linkwitz-Rileyフィルター（複数段でスロープを実現）
    // 12dB/oct = 1段, 24dB/oct = 1段(LR), 48dB/oct = 2段, 96dB = 4段, 192dB = 8段
    std::array<juce::dsp::LinkwitzRileyFilter<float>, 8> lowpassFilters;
    std::array<juce::dsp::LinkwitzRileyFilter<float>, 8> highpassFilters;

    // クロスオーバー付近のゲイン補正用ピークEQ（ステレオ）
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> compensationFilter;

    // スペクトラムアナライザー
    SpectrumAnalyzer spectrumAnalyzer;

    // サンプルレート保存
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassSplitterAudioProcessor)
};
