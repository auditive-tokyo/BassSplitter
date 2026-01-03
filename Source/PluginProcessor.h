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

private:
    // パラメータレイアウト作成
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // パラメータ管理
    juce::AudioProcessorValueTreeState apvts;

    // Linkwitz-Rileyフィルター（ローパス・ハイパス）
    juce::dsp::LinkwitzRileyFilter<float> lowpassFilter;
    juce::dsp::LinkwitzRileyFilter<float> highpassFilter;

    // スペクトラムアナライザー
    SpectrumAnalyzer spectrumAnalyzer;

    // サンプルレート保存
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassSplitterAudioProcessor)
};
