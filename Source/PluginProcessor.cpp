#include "PluginProcessor.h"
#include "PluginEditor.h"

BassSplitterAudioProcessor::BassSplitterAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Low", juce::AudioChannelSet::stereo(), true)
                         .withOutput("High", juce::AudioChannelSet::stereo(), true))
{
    lowpassFilter.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    highpassFilter.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
}

BassSplitterAudioProcessor::~BassSplitterAudioProcessor()
{
}

const juce::String BassSplitterAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool BassSplitterAudioProcessor::acceptsMidi() const
{
    return false;
}

bool BassSplitterAudioProcessor::producesMidi() const
{
    return false;
}

bool BassSplitterAudioProcessor::isMidiEffect() const
{
    return false;
}

double BassSplitterAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int BassSplitterAudioProcessor::getNumPrograms()
{
    return 1;
}

int BassSplitterAudioProcessor::getCurrentProgram()
{
    return 0;
}

void BassSplitterAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String BassSplitterAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void BassSplitterAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void BassSplitterAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumInputChannels());

    lowpassFilter.prepare(spec);
    highpassFilter.prepare(spec);

    lowpassFilter.setCutoffFrequency(crossoverFrequency);
    highpassFilter.setCutoffFrequency(crossoverFrequency);
}

void BassSplitterAudioProcessor::releaseResources()
{
    lowpassFilter.reset();
    highpassFilter.reset();
}

bool BassSplitterAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // 入力はステレオ
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // 出力は2つのステレオバス（Low, High）
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void BassSplitterAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // 未使用の出力チャンネルをクリア
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // 入力をコピーしてフィルタリング
    juce::AudioBuffer<float> lowBuffer(buffer.getNumChannels(), buffer.getNumSamples());
    juce::AudioBuffer<float> highBuffer(buffer.getNumChannels(), buffer.getNumSamples());

    // 入力信号をコピー
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        lowBuffer.copyFrom(channel, 0, buffer, channel, 0, buffer.getNumSamples());
        highBuffer.copyFrom(channel, 0, buffer, channel, 0, buffer.getNumSamples());
    }

    // Lowpassフィルタを適用
    juce::dsp::AudioBlock<float> lowBlock(lowBuffer);
    juce::dsp::ProcessContextReplacing<float> lowContext(lowBlock);
    lowpassFilter.process(lowContext);

    // Highpassフィルタを適用
    juce::dsp::AudioBlock<float> highBlock(highBuffer);
    juce::dsp::ProcessContextReplacing<float> highContext(highBlock);
    highpassFilter.process(highContext);

    // 出力バッファに結果を書き込む
    // シンプル版：LowとHighを合成して出力（分割確認用）
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* output = buffer.getWritePointer(channel);
        auto* low = lowBuffer.getReadPointer(channel);
        auto* high = highBuffer.getReadPointer(channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            // Low + High を合成（位相が揃うので元の信号に戻るはず）
            output[sample] = low[sample] + high[sample];
        }
    }
}

bool BassSplitterAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* BassSplitterAudioProcessor::createEditor()
{
    return new BassSplitterAudioProcessorEditor(*this);
}

void BassSplitterAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::ignoreUnused(destData);
}

void BassSplitterAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    juce::ignoreUnused(data, sizeInBytes);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginInstance()
{
    return new BassSplitterAudioProcessor();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BassSplitterAudioProcessor();
}
