#include "BandProcessor.h"

#include <numbers>

void BandProcessor::processSingleBand(const BandParameters& params, const BandProcessingContext& context) const
{
    applyEQFilters(context.bandBuffer, context.bandEQ, context.numStages);

    bool isMono = params.mono[static_cast<size_t>(context.band)] && context.numChannels >= 2;
    if (isMono)
        applyMonoProcessing(context.bandBuffer, context.numSamples);

    float peakL = 0.0f;
    float peakR = 0.0f;

    if (isMono)
        calculateMonoPeaks(context.bandBuffer, context.numSamples, peakL, peakR);

    applyPanning(context.bandBuffer, params.pans[static_cast<size_t>(context.band)], context.numChannels, context.numSamples);

    if (!isMono)
        calculateStereoPeaks(context.bandBuffer, context.numChannels, context.numSamples, peakL, peakR);

    applyGainAndStorePeaks(params.gains[static_cast<size_t>(context.band)], peakL, peakR, context.peakLevelL, context.peakLevelR);
}

void BandProcessor::applyEQFilters(juce::AudioBuffer<float>& bandBuffer, BandEQ& bandEQ, int numStages) const
{
    juce::dsp::AudioBlock<float> block(bandBuffer);

    for (int stage = 0; stage < numStages; ++stage)
    {
        juce::dsp::ProcessContextReplacing ctx(block);
        bandEQ.highpass[static_cast<size_t>(stage)].process(ctx);
    }

    for (int stage = 0; stage < numStages; ++stage)
    {
        juce::dsp::ProcessContextReplacing ctx(block);
        bandEQ.lowpass[static_cast<size_t>(stage)].process(ctx);
    }
}

void BandProcessor::applyMonoProcessing(juce::AudioBuffer<float>& bandBuffer, int numSamples) const
{
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

void BandProcessor::calculateMonoPeaks(const juce::AudioBuffer<float>& bandBuffer,
                                       int numSamples,
                                       float& peakL,
                                       float& peakR) const
{
    auto rangeL = juce::FloatVectorOperations::findMinAndMax(bandBuffer.getReadPointer(0), numSamples);
    peakL = std::max(std::abs(rangeL.getStart()), std::abs(rangeL.getEnd()));
    peakR = peakL; // モノなので同じ
}

void BandProcessor::applyPanning(juce::AudioBuffer<float>& bandBuffer,
                                 float panValue,
                                 int numChannels,
                                 int numSamples) const
{
    if (numChannels < 2)
        return;

    float panNormalized = panValue / 100.0f; // -1.0 to +1.0
    if (std::abs(panNormalized) <= 0.001f)
        return; // センター、処理不要

    // 等パワーパンニングを適用
    float angle = (panNormalized + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
    float gainL = std::cos(angle);
    float gainR = std::sin(angle);

    float* leftWrite = bandBuffer.getWritePointer(0);
    float* rightWrite = bandBuffer.getWritePointer(1);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float left = leftWrite[sample];
        float right = rightWrite[sample];
        float monoMix = (left + right) * 0.5f;
        leftWrite[sample] = monoMix * gainL * std::numbers::sqrt2_v<float>; // √2で補正（センターで0dB）
        rightWrite[sample] = monoMix * gainR * std::numbers::sqrt2_v<float>;
    }
}

void BandProcessor::calculateStereoPeaks(
    const juce::AudioBuffer<float>& bandBuffer, int numChannels, int numSamples, float& peakL, float& peakR) const
{
    if (numChannels >= 1)
    {
        auto rangeL = juce::FloatVectorOperations::findMinAndMax(bandBuffer.getReadPointer(0), numSamples);
        peakL = std::max(std::abs(rangeL.getStart()), std::abs(rangeL.getEnd()));
    }

    if (numChannels >= 2)
    {
        auto rangeR = juce::FloatVectorOperations::findMinAndMax(bandBuffer.getReadPointer(1), numSamples);
        peakR = std::max(std::abs(rangeR.getStart()), std::abs(rangeR.getEnd()));
    }
    else
    {
        peakR = peakL;
    }
}

void BandProcessor::applyGainAndStorePeaks(
    float gain, float peakL, float peakR, std::atomic<float>& peakLevelL, std::atomic<float>& peakLevelR) const
{
    peakL *= gain;
    peakR *= gain;
    peakLevelL.store(peakL);
    peakLevelR.store(peakR);
}
