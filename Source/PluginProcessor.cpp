#include "PluginProcessor.h"

#include "PluginEditor.h"

#include <span>

BassSplitterAudioProcessor::BassSplitterAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // フィルタータイプを設定
    for (auto& bandEQ : bandEQFilters)
    {
        for (auto& filter : bandEQ.highpass)
            filter.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
        for (auto& filter : bandEQ.lowpass)
            filter.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    }

    // ピークレベルを初期化
    for (auto& level : bandPeakLevelsL)
        level.store(0.0f);
    for (auto& level : bandPeakLevelsR)
        level.store(0.0f);
}

juce::AudioProcessorValueTreeState::ParameterLayout BassSplitterAudioProcessor::createParameterLayout() const
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // グローバルスロープパラメータ
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("slope", 1),
        "Slope",
        juce::StringArray{"12 dB/oct", "24 dB/oct", "48 dB/oct", "96 dB/oct", "192 dB/oct"},
        1 // デフォルト: 24dB/oct
        ));

    // ゲイン表示用のラムダ
    auto gainToString = [](float value, int)
    {
        if (value <= -69.5f)
            return juce::String(juce::CharPointer_UTF8("-\xe2\x88\x9e"));
        return juce::String(value, 1) + " dB";
    };

    // 各バンドのパラメータ
    for (int i = 0; i < numBands; ++i)
    {
        juce::String bandId = "band" + juce::String(i + 1);
        juce::String bandName = "Band " + juce::String(i + 1);

        // Bypass（デフォルトではBand2-5がバイパス）
        bool defaultBypass = (i >= 1 && i <= 4); // Band 2, 3, 4, 5
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID(bandId + "Bypass", 1), bandName + " Bypass", defaultBypass));

        // Solo
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID(bandId + "Solo", 1), bandName + " Solo", false));

        // Mono
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID(bandId + "Mono", 1), bandName + " Mono", false));

        // Pan parameter: -100 (Left), 0 (Center), +100 (Right)
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(bandId + "Pan", 1),
            bandName + " Pan",
            juce::NormalisableRange<float>(-100.0f, 100.0f, 1.0f),
            0.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction(
                [](float value, int)
                {
                    if (value < -0.5f)
                        return juce::String(static_cast<int>(-value)) + "L";
                    if (value > 0.5f)
                        return juce::String(static_cast<int>(value)) + "R";
                    return juce::String("C");
                })));

        // Gain
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(bandId + "Gain", 1),
            bandName + " Gain",
            juce::NormalisableRange<float>(-70.0f, 6.0f, 0.1f, 2.5f),
            0.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction(gainToString)));

        // Highpass EQ Frequency（0-20kHz、デフォルト0Hz = フィルタリングなし）
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(bandId + "HighpassFreq", 1),
                                                        bandName + " Highpass Freq",
                                                        juce::NormalisableRange<float>(0.0f, 20000.0f, 1.0f, 0.3f),
                                                        0.0f,
                                                        "Hz"));

        // Lowpass EQ Frequency（0-20kHz、デフォルト20kHz = フィルタリングなし）
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(bandId + "LowpassFreq", 1),
                                                        bandName + " Lowpass Freq",
                                                        juce::NormalisableRange<float>(0.0f, 20000.0f, 1.0f, 0.3f),
                                                        20000.0f,
                                                        "Hz"));
    }

    return {params.begin(), params.end()};
}

BassSplitterAudioProcessor::~BassSplitterAudioProcessor() = default;

const juce::String BassSplitterAudioProcessor::getName() const // NOSONAR - JUCE API requires const return type.
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

const juce::String BassSplitterAudioProcessor::getProgramName(int index) // NOSONAR - JUCE API requires const return type.
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

    // フィルターを準備
    for (auto& bandEQ : bandEQFilters)
    {
        for (auto& filter : bandEQ.highpass)
            filter.prepare(spec);
        for (auto& filter : bandEQ.lowpass)
            filter.prepare(spec);
    }

    // バンドバッファを準備
    for (auto& buffer : bandBuffers)
        buffer.setSize(static_cast<int>(spec.numChannels), samplesPerBlock);

    // スペクトラムアナライザーを設定
    spectrumAnalyzer.setSampleRate(sampleRate);
}

void BassSplitterAudioProcessor::releaseResources()
{
    for (auto& bandEQ : bandEQFilters)
    {
        for (auto& filter : bandEQ.highpass)
            filter.reset();
        for (auto& filter : bandEQ.lowpass)
            filter.reset();
    }
}

bool BassSplitterAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void BassSplitterAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    auto numChannels = buffer.getNumChannels();
    auto numSamples = buffer.getNumSamples();

    // スペクトラムアナライザーに入力信号を送る
    if (numChannels > 0)
        spectrumAnalyzer.pushSamples(
            std::span<const float>(buffer.getReadPointer(0), static_cast<size_t>(numSamples)));

    auto slopeIndex = static_cast<int>(apvts.getRawParameterValue("slope")->load());
    int numStages = ProcessBlockCoordinator::getNumFilterStages(slopeIndex);
    blockCoordinator.updateEQFilterSettings(apvts, bandEQFilters);
    
    auto params = blockCoordinator.loadBandParameters(apvts);
    blockCoordinator.prepareBandBuffers(buffer, bandBuffers, numChannels, numSamples);
    
    for (int band = 0; band < numBands; ++band)
    {
        if (!params.bypassed[static_cast<size_t>(band)])
        {
            BandProcessor::BandProcessingContext context{
                band,
                numStages,
                numChannels,
                numSamples,
                bandBuffers[static_cast<size_t>(band)],
                bandEQFilters[static_cast<size_t>(band)],
                bandPeakLevelsL[static_cast<size_t>(band)],
                bandPeakLevelsR[static_cast<size_t>(band)]
            };
            bandProcessor.processSingleBand(params, context);
        }
    }
    
    blockCoordinator.clearBypassedBandPeaks(params, bandPeakLevelsL, bandPeakLevelsR);
    blockCoordinator.mixBandsToOutput(buffer, params, bandBuffers, numChannels, numSamples);
}

bool BassSplitterAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* BassSplitterAudioProcessor::createEditor()
{
    return new BassSplitterAudioProcessorEditor(*this); // NOSONAR - JUCE owns editor lifetime.
}

void BassSplitterAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    // バンド名を保存
    for (int i = 0; i < numBands; ++i)
    {
        state.setProperty("bandName" + juce::String(i), bandNames[static_cast<size_t>(i)], nullptr);
    }

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void BassSplitterAudioProcessor::setStateInformation(const void* data, // NOSONAR - JUCE API override uses void*.
                                                     int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
    {
        auto state = juce::ValueTree::fromXml(*xmlState);

        // バンド名を復元
        for (int i = 0; i < numBands; ++i)
        {
            auto name = state.getProperty("bandName" + juce::String(i), "Band " + juce::String(i + 1));
            bandNames[static_cast<size_t>(i)] = name.toString();
        }

        apvts.replaceState(state);
    }
}

int BassSplitterAudioProcessor::getCurrentSlopeDB() const
{
    auto slopeIndex = static_cast<int>(apvts.getRawParameterValue("slope")->load());
    switch (slopeIndex)
    {
    case 0:
        return 12;
    case 1:
        return 24;
    case 2:
        return 48;
    case 3:
        return 96;
    case 4:
        return 192;
    default:
        return 24;
    }
}

float BassSplitterAudioProcessor::getBandHighpassFreq(int bandIndex) const
{
    if (bandIndex < 0 || bandIndex >= numBands)
        return 0.0f;
    juce::String bandId = "band" + juce::String(bandIndex + 1);
    return apvts.getRawParameterValue(bandId + "HighpassFreq")->load();
}

float BassSplitterAudioProcessor::getBandLowpassFreq(int bandIndex) const
{
    if (bandIndex < 0 || bandIndex >= numBands)
        return 20000.0f;
    juce::String bandId = "band" + juce::String(bandIndex + 1);
    return apvts.getRawParameterValue(bandId + "LowpassFreq")->load();
}

juce::String BassSplitterAudioProcessor::getBandName(int bandIndex) const
{
    if (bandIndex < 0 || bandIndex >= numBands)
        return "Band";
    return bandNames[static_cast<size_t>(bandIndex)];
}

void BassSplitterAudioProcessor::setBandName(int bandIndex, const juce::String& name)
{
    if (bandIndex >= 0 && bandIndex < numBands)
        bandNames[static_cast<size_t>(bandIndex)] = name;
}

float BassSplitterAudioProcessor::getBandPeakLevel(int bandIndex) const
{
    if (bandIndex < 0 || bandIndex >= numBands)
        return 0.0f;
    // 左右の最大値を返す
    return std::max(bandPeakLevelsL[static_cast<size_t>(bandIndex)].load(),
                    bandPeakLevelsR[static_cast<size_t>(bandIndex)].load());
}

void BassSplitterAudioProcessor::getBandPeakLevelStereo(int bandIndex, float& leftLevel, float& rightLevel) const
{
    if (bandIndex < 0 || bandIndex >= numBands)
    {
        leftLevel = 0.0f;
        rightLevel = 0.0f;
        return;
    }
    leftLevel = bandPeakLevelsL[static_cast<size_t>(bandIndex)].load();
    rightLevel = bandPeakLevelsR[static_cast<size_t>(bandIndex)].load();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BassSplitterAudioProcessor(); // NOSONAR - JUCE/host owns deletion of plugin instance.
}
