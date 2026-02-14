#include "PluginProcessor.h"

#include "PluginEditor.h"

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

juce::AudioProcessorValueTreeState::ParameterLayout BassSplitterAudioProcessor::createParameterLayout()
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

        // Pan (-100 = Left, 0 = Center, +100 = Right)
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

BassSplitterAudioProcessor::~BassSplitterAudioProcessor() {}

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
        spectrumAnalyzer.pushSamples(buffer.getReadPointer(0), numSamples);

    // スロープを取得
    auto slopeIndex = static_cast<int>(apvts.getRawParameterValue("slope")->load());
    int numStages = 1;
    switch (slopeIndex)
    {
    case 0:
        numStages = 1;
        break; // 12dB/oct
    case 1:
        numStages = 1;
        break; // 24dB/oct
    case 2:
        numStages = 2;
        break; // 48dB/oct
    case 3:
        numStages = 4;
        break; // 96dB/oct
    case 4:
        numStages = 8;
        break; // 192dB/oct
    default:
        numStages = 1;
        break;
    }

    // 各バンドのEQ周波数を取得してフィルターに設定
    for (int i = 0; i < numBands; ++i)
    {
        juce::String bandId = "band" + juce::String(i + 1);
        float highpassFreq = apvts.getRawParameterValue(bandId + "HighpassFreq")->load();
        float lowpassFreq = apvts.getRawParameterValue(bandId + "LowpassFreq")->load();

        // フィルターに周波数を設定
        for (auto& filter : bandEQFilters[static_cast<size_t>(i)].highpass)
            filter.setCutoffFrequency(highpassFreq);
        for (auto& filter : bandEQFilters[static_cast<size_t>(i)].lowpass)
            filter.setCutoffFrequency(lowpassFreq);
    }

    // バンドパラメータを取得
    std::array<bool, numBands> bypassed;
    std::array<bool, numBands> soloed;
    std::array<bool, numBands> mono;
    std::array<float, numBands> pans;
    std::array<float, numBands> gains;
    bool anySolo = false;

    for (int i = 0; i < numBands; ++i)
    {
        juce::String bandId = "band" + juce::String(i + 1);
        bypassed[static_cast<size_t>(i)] = apvts.getRawParameterValue(bandId + "Bypass")->load() > 0.5f;
        soloed[static_cast<size_t>(i)] = apvts.getRawParameterValue(bandId + "Solo")->load() > 0.5f;
        mono[static_cast<size_t>(i)] = apvts.getRawParameterValue(bandId + "Mono")->load() > 0.5f;
        pans[static_cast<size_t>(i)] = apvts.getRawParameterValue(bandId + "Pan")->load();
        float gainDB = apvts.getRawParameterValue(bandId + "Gain")->load();
        gains[static_cast<size_t>(i)] = juce::Decibels::decibelsToGain(gainDB);

        if (soloed[static_cast<size_t>(i)])
            anySolo = true;
    }

    // 各バンドを処理
    // 各バンドは独立したハイパス→ローパスのEQフィルターを持つ

    // バンドバッファを準備
    for (int band = 0; band < numBands; ++band)
    {
        bandBuffers[static_cast<size_t>(band)].setSize(numChannels, numSamples, false, false, true);

        // 入力をコピー
        for (int ch = 0; ch < numChannels; ++ch)
            bandBuffers[static_cast<size_t>(band)].copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }

    // 各バンドにフィルターを適用
    for (int band = 0; band < numBands; ++band)
    {
        if (bypassed[static_cast<size_t>(band)])
            continue; // バイパス時は処理スキップ

        juce::dsp::AudioBlock<float> block(bandBuffers[static_cast<size_t>(band)]);

        // ハイパスフィルターを適用
        for (int stage = 0; stage < numStages; ++stage)
        {
            juce::dsp::ProcessContextReplacing<float> ctx(block);
            bandEQFilters[static_cast<size_t>(band)].highpass[static_cast<size_t>(stage)].process(ctx);
        }

        // ローパスフィルターを適用
        for (int stage = 0; stage < numStages; ++stage)
        {
            juce::dsp::ProcessContextReplacing<float> ctx(block);
            bandEQFilters[static_cast<size_t>(band)].lowpass[static_cast<size_t>(stage)].process(ctx);
        }

        // モノ処理: ステレオをモノにサムして両チャンネルに書き込む
        bool isMono = mono[static_cast<size_t>(band)] && numChannels >= 2;
        if (isMono)
        {
            auto& bandBuffer = bandBuffers[static_cast<size_t>(band)];
            const float* leftData = bandBuffer.getReadPointer(0);
            const float* rightData = bandBuffer.getReadPointer(1);
            float* leftWrite = bandBuffer.getWritePointer(0);
            float* rightWrite = bandBuffer.getWritePointer(1);

            for (int sample = 0; sample < numSamples; ++sample)
            {
                float monoSample = (leftData[sample] + rightData[sample]) * 0.5f;
                leftWrite[sample] = monoSample;
                rightWrite[sample] = monoSample;
            }
        }

        // Mono時: パン処理前にピークを計算（パンに関係なく一定のレベル表示）
        float peakL = 0.0f;
        float peakR = 0.0f;

        if (isMono)
        {
            // モノの場合はパン前に計算（L=Rなので同じ値）
            auto rangeL = juce::FloatVectorOperations::findMinAndMax(
                bandBuffers[static_cast<size_t>(band)].getReadPointer(0), numSamples);
            peakL = std::max(std::abs(rangeL.getStart()), std::abs(rangeL.getEnd()));
            peakR = peakL; // モノなので同じ
        }

        // パン処理: 等パワーパンニング
        if (numChannels >= 2)
        {
            float panValue = pans[static_cast<size_t>(band)] / 100.0f; // -1.0 to +1.0
            // パンが0（センター）でない場合のみ処理
            if (std::abs(panValue) > 0.001f)
            {
                // 等パワーパンニング: L = cos(angle), R = sin(angle)
                // angle: 0 (left) to π/2 (right), center = π/4
                float angle = (panValue + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
                float gainL = std::cos(angle);
                float gainR = std::sin(angle);

                auto& bandBuffer = bandBuffers[static_cast<size_t>(band)];
                float* leftWrite = bandBuffer.getWritePointer(0);
                float* rightWrite = bandBuffer.getWritePointer(1);

                for (int sample = 0; sample < numSamples; ++sample)
                {
                    float left = leftWrite[sample];
                    float right = rightWrite[sample];
                    // モノソース（または一般的なパン）用: 両チャンネルをミックスしてパンを適用
                    float monoMix = (left + right) * 0.5f;
                    leftWrite[sample] = monoMix * gainL * 1.414f; // √2で補正（センターで0dB）
                    rightWrite[sample] = monoMix * gainR * 1.414f;
                }
            }
        }

        // Stereo時: パン処理後にピークを計算（L/Rの実際のレベルを表示）
        if (!isMono)
        {
            if (numChannels >= 1)
            {
                auto rangeL = juce::FloatVectorOperations::findMinAndMax(
                    bandBuffers[static_cast<size_t>(band)].getReadPointer(0), numSamples);
                peakL = std::max(std::abs(rangeL.getStart()), std::abs(rangeL.getEnd()));
            }
            if (numChannels >= 2)
            {
                auto rangeR = juce::FloatVectorOperations::findMinAndMax(
                    bandBuffers[static_cast<size_t>(band)].getReadPointer(1), numSamples);
                peakR = std::max(std::abs(rangeR.getStart()), std::abs(rangeR.getEnd()));
            }
            else
            {
                peakR = peakL;
            }
        }

        // ゲインを適用してメーター用に保存
        peakL *= gains[static_cast<size_t>(band)];
        peakR *= gains[static_cast<size_t>(band)];
        bandPeakLevelsL[static_cast<size_t>(band)].store(peakL);
        bandPeakLevelsR[static_cast<size_t>(band)].store(peakR);
    }

    // バイパス中のバンドはピークをゼロに
    for (int band = 0; band < numBands; ++band)
    {
        if (bypassed[static_cast<size_t>(band)])
        {
            bandPeakLevelsL[static_cast<size_t>(band)].store(0.0f);
            bandPeakLevelsR[static_cast<size_t>(band)].store(0.0f);
        }
    }

    // 出力をミックス
    buffer.clear();
    for (int band = 0; band < numBands; ++band)
    {
        // バイパス時はスキップ
        if (bypassed[static_cast<size_t>(band)])
            continue;

        // ソロモード：ソロされていないバンドはスキップ
        if (anySolo && !soloed[static_cast<size_t>(band)])
            continue;

        float gain = gains[static_cast<size_t>(band)];

        for (int ch = 0; ch < numChannels; ++ch)
        {
            buffer.addFrom(ch, 0, bandBuffers[static_cast<size_t>(band)], ch, 0, numSamples, gain);
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
    auto state = apvts.copyState();

    // バンド名を保存
    for (int i = 0; i < numBands; ++i)
    {
        state.setProperty("bandName" + juce::String(i), bandNames[static_cast<size_t>(i)], nullptr);
    }

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void BassSplitterAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
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
    return new BassSplitterAudioProcessor();
}
