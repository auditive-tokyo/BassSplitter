#include "PluginProcessor.h"
#include "PluginEditor.h"

BassSplitterAudioProcessor::BassSplitterAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    for (auto& filter : lowpassFilters)
        filter.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    for (auto& filter : highpassFilters)
        filter.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
}

juce::AudioProcessorValueTreeState::ParameterLayout BassSplitterAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // クロスオーバー周波数パラメータ（20Hz〜2000Hz、デフォルト200Hz）
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("crossover", 1),  // パラメータID
        "Crossover",                         // 表示名
        juce::NormalisableRange<float>(20.0f, 2000.0f, 1.0f, 0.3f),  // 範囲（対数スケール）
        200.0f,                              // デフォルト値
        "Hz"                                 // 単位
    ));

    // スロープパラメータ（0=12dB, 1=24dB, 2=48dB, 3=96dB, 4=192dB）
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("slope", 1),
        "Slope",
        juce::StringArray{ "12 dB/oct", "24 dB/oct", "48 dB/oct", "96 dB/oct", "192 dB/oct" },
        1  // デフォルト: 24dB/oct
    ));

    // Low Band Solo
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("lowSolo", 1),
        "Low Solo",
        false
    ));

    // High Band Solo
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("highSolo", 1),
        "High Solo",
        false
    ));

    // Low Band Gain (-inf 〜 +6dB)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("lowGain", 1),
        "Low Gain",
        juce::NormalisableRange<float>(-70.0f, 6.0f, 0.1f, 2.5f),
        0.0f,
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction([](float value, int) {
                if (value <= -69.5f) return juce::String(juce::CharPointer_UTF8("-\xe2\x88\x9e"));
                return juce::String(value, 1) + " dB";
            })
    ));

    // High Band Gain (-inf 〜 +6dB)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("highGain", 1),
        "High Gain",
        juce::NormalisableRange<float>(-70.0f, 6.0f, 0.1f, 2.5f),
        0.0f,
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction([](float value, int) {
                if (value <= -69.5f) return juce::String(juce::CharPointer_UTF8("-\xe2\x88\x9e"));
                return juce::String(value, 1) + " dB";
            })
    ));

    return { params.begin(), params.end() };
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
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumInputChannels());

    for (auto& filter : lowpassFilters)
        filter.prepare(spec);
    for (auto& filter : highpassFilters)
        filter.prepare(spec);

    // 補正フィルターを準備
    compensationFilter.prepare(spec);

    // 初期周波数を設定
    float freq = apvts.getRawParameterValue("crossover")->load();
    for (auto& filter : lowpassFilters)
        filter.setCutoffFrequency(freq);
    for (auto& filter : highpassFilters)
        filter.setCutoffFrequency(freq);

    // スペクトラムアナライザーを設定
    spectrumAnalyzer.setSampleRate(sampleRate);
}

void BassSplitterAudioProcessor::releaseResources()
{
    for (auto& filter : lowpassFilters)
        filter.reset();
    for (auto& filter : highpassFilters)
        filter.reset();
    compensationFilter.reset();
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

    // パラメータから周波数とスロープを取得
    float freq = apvts.getRawParameterValue("crossover")->load();
    int slopeIndex = static_cast<int>(apvts.getRawParameterValue("slope")->load());
    
    // 全フィルターに周波数を設定
    for (auto& filter : lowpassFilters)
        filter.setCutoffFrequency(freq);
    for (auto& filter : highpassFilters)
        filter.setCutoffFrequency(freq);

    // スロープに応じたフィルター段数を決定
    // 0=12dB(1段), 1=24dB(1段), 2=48dB(2段), 3=96dB(4段), 4=192dB(8段)
    int numStages = 1;
    float peakGainDB = 0.0f;  // ピークEQの補正量
    float peakQ = 0.7f;       // ピークEQのQ値
    switch (slopeIndex)
    {
        case 0: numStages = 1; peakGainDB = 0.0f; break;   // 12dB/oct - 補正なし
        case 1: numStages = 1; peakGainDB = 0.0f; break;   // 24dB/oct - 補正なし（基準）
        case 2: numStages = 2; peakGainDB = 3.0f; peakQ = 0.7f; break;   // 48dB/oct
        case 3: numStages = 4; peakGainDB = 6.0f; peakQ = 0.5f; break;   // 96dB/oct
        case 4: numStages = 8; peakGainDB = 9.0f; peakQ = 0.4f; break;   // 192dB/oct
        default: numStages = 1; peakGainDB = 0.0f; break;
    }

    // クロスオーバー付近の補正フィルターを更新
    if (peakGainDB > 0.0f)
    {
        *compensationFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            currentSampleRate, freq, peakQ, juce::Decibels::decibelsToGain(peakGainDB));
    }
    else
    {
        // 補正なしの場合はフラットに
        *compensationFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            currentSampleRate, freq, 0.7f, 1.0f);
    }

    // 未使用の出力チャンネルをクリア
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // スペクトラムアナライザーに入力信号を送る（左チャンネル）
    if (buffer.getNumChannels() > 0)
        spectrumAnalyzer.pushSamples(buffer.getReadPointer(0), buffer.getNumSamples());

    // 入力をコピーしてフィルタリング
    juce::AudioBuffer<float> lowBuffer(buffer.getNumChannels(), buffer.getNumSamples());
    juce::AudioBuffer<float> highBuffer(buffer.getNumChannels(), buffer.getNumSamples());

    // 入力信号をコピー
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        lowBuffer.copyFrom(channel, 0, buffer, channel, 0, buffer.getNumSamples());
        highBuffer.copyFrom(channel, 0, buffer, channel, 0, buffer.getNumSamples());
    }

    // Lowpassフィルタを適用（指定段数分）
    juce::dsp::AudioBlock<float> lowBlock(lowBuffer);
    for (int stage = 0; stage < numStages; ++stage)
    {
        juce::dsp::ProcessContextReplacing<float> lowContext(lowBlock);
        lowpassFilters[static_cast<size_t>(stage)].process(lowContext);
    }

    // Highpassフィルタを適用（指定段数分）
    juce::dsp::AudioBlock<float> highBlock(highBuffer);
    for (int stage = 0; stage < numStages; ++stage)
    {
        juce::dsp::ProcessContextReplacing<float> highContext(highBlock);
        highpassFilters[static_cast<size_t>(stage)].process(highContext);
    }

    // 出力バッファに結果を書き込む
    // Solo状態とゲインを取得
    bool lowSolo = apvts.getRawParameterValue("lowSolo")->load() > 0.5f;
    bool highSolo = apvts.getRawParameterValue("highSolo")->load() > 0.5f;
    float lowGainDB = apvts.getRawParameterValue("lowGain")->load();
    float highGainDB = apvts.getRawParameterValue("highGain")->load();
    float lowGain = juce::Decibels::decibelsToGain(lowGainDB);
    float highGain = juce::Decibels::decibelsToGain(highGainDB);

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* output = buffer.getWritePointer(channel);
        auto* low = lowBuffer.getReadPointer(channel);
        auto* high = highBuffer.getReadPointer(channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            // ゲインを適用
            float lowSample = low[sample] * lowGain;
            float highSample = high[sample] * highGain;
            
            float result;
            
            // Solo状態に応じて出力を切り替え
            if (lowSolo && !highSolo)
            {
                // Low Solo: Lowだけ出力
                result = lowSample;
            }
            else if (highSolo && !lowSolo)
            {
                // High Solo: Highだけ出力
                result = highSample;
            }
            else
            {
                // 両方Soloまたは両方Off: Low + High を合成
                result = lowSample + highSample;
            }
            
            output[sample] = result;
        }
    }

    // 補正フィルターを適用（クロスオーバー付近のディップを補正）
    if (peakGainDB > 0.0f)
    {
        juce::dsp::AudioBlock<float> outputBlock(buffer);
        juce::dsp::ProcessContextReplacing<float> context(outputBlock);
        compensationFilter.process(context);
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
    // パラメータの状態を保存
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void BassSplitterAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // パラメータの状態を復元
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

int BassSplitterAudioProcessor::getCurrentSlopeDB() const
{
    int slopeIndex = static_cast<int>(apvts.getRawParameterValue("slope")->load());
    switch (slopeIndex)
    {
        case 0: return 12;
        case 1: return 24;
        case 2: return 48;
        case 3: return 96;
        case 4: return 192;
        default: return 24;
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginInstance()
{
    return new BassSplitterAudioProcessor();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BassSplitterAudioProcessor();
}
