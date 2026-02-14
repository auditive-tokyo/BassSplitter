#include "ProcessBlockCoordinator.h"

int ProcessBlockCoordinator::getNumFilterStages(int slopeIndex)
{
    switch (slopeIndex)
    {
    case 0: return 1;  // 12dB/oct
    case 1: return 1;  // 24dB/oct
    case 2: return 2;  // 48dB/oct
    case 3: return 4;  // 96dB/oct
    case 4: return 8;  // 192dB/oct
    default: return 1;
    }
}

void ProcessBlockCoordinator::updateEQFilterSettings(const juce::AudioProcessorValueTreeState& apvts,
                                                     std::array<BandProcessor::BandEQ, 6>& bandEQFilters) const
{
    for (int i = 0; i < 6; ++i)
    {
        juce::String bandId = "band" + juce::String(i + 1);
        float highpassFreq = apvts.getRawParameterValue(bandId + "HighpassFreq")->load();
        float lowpassFreq = apvts.getRawParameterValue(bandId + "LowpassFreq")->load();

        for (auto& filter : bandEQFilters[static_cast<size_t>(i)].highpass)
            filter.setCutoffFrequency(highpassFreq);
        for (auto& filter : bandEQFilters[static_cast<size_t>(i)].lowpass)
            filter.setCutoffFrequency(lowpassFreq);
    }
}

ProcessBlockCoordinator::BandParameters ProcessBlockCoordinator::loadBandParameters(
    const juce::AudioProcessorValueTreeState& apvts) const
{
    BandParameters params;
    params.anySolo = false;

    for (int i = 0; i < 6; ++i)
    {
        juce::String bandId = "band" + juce::String(i + 1);
        params.bypassed[static_cast<size_t>(i)] = apvts.getRawParameterValue(bandId + "Bypass")->load() > 0.5f;
        params.soloed[static_cast<size_t>(i)] = apvts.getRawParameterValue(bandId + "Solo")->load() > 0.5f;
        params.mono[static_cast<size_t>(i)] = apvts.getRawParameterValue(bandId + "Mono")->load() > 0.5f;
        params.pans[static_cast<size_t>(i)] = apvts.getRawParameterValue(bandId + "Pan")->load();
        float gainDB = apvts.getRawParameterValue(bandId + "Gain")->load();
        params.gains[static_cast<size_t>(i)] = juce::Decibels::decibelsToGain(gainDB);

        if (params.soloed[static_cast<size_t>(i)])
            params.anySolo = true;
    }

    return params;
}

void ProcessBlockCoordinator::prepareBandBuffers(const juce::AudioBuffer<float>& buffer,
                                                 std::array<juce::AudioBuffer<float>, 6>& bandBuffers,
                                                 int numChannels,
                                                 int numSamples) const
{
    for (int band = 0; band < 6; ++band)
    {
        bandBuffers[static_cast<size_t>(band)].setSize(numChannels, numSamples, false, false, true);

        for (int ch = 0; ch < numChannels; ++ch)
            bandBuffers[static_cast<size_t>(band)].copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }
}

void ProcessBlockCoordinator::clearBypassedBandPeaks(const BandParameters& params,
                                                     std::array<std::atomic<float>, 6>& bandPeakLevelsL,
                                                     std::array<std::atomic<float>, 6>& bandPeakLevelsR) const
{
    for (int band = 0; band < 6; ++band)
    {
        if (params.bypassed[static_cast<size_t>(band)])
        {
            bandPeakLevelsL[static_cast<size_t>(band)].store(0.0f);
            bandPeakLevelsR[static_cast<size_t>(band)].store(0.0f);
        }
    }
}

void ProcessBlockCoordinator::mixBandsToOutput(juce::AudioBuffer<float>& buffer,
                                               const BandParameters& params,
                                               const std::array<juce::AudioBuffer<float>, 6>& bandBuffers,
                                               int numChannels,
                                               int numSamples) const
{
    buffer.clear();

    for (int band = 0; band < 6; ++band)
    {
        if (params.bypassed[static_cast<size_t>(band)])
            continue;

        if (params.anySolo && !params.soloed[static_cast<size_t>(band)])
            continue;

        float gain = params.gains[static_cast<size_t>(band)];

        for (int ch = 0; ch < numChannels; ++ch)
        {
            buffer.addFrom(ch, 0, bandBuffers[static_cast<size_t>(band)], ch, 0, numSamples, gain);
        }
    }
}
