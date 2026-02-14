#include "EQCoordinateMapper.h"

#include <cmath>

float EQCoordinateMapper::frequencyToX(float freq, int width) const
{
    float normalized = (std::log10(freq) - std::log10(minFreq)) / (std::log10(maxFreq) - std::log10(minFreq));
    return normalized * static_cast<float>(width);
}

float EQCoordinateMapper::xToFrequency(float x, int width) const
{
    float normalized = x / static_cast<float>(width);
    return std::pow(10.0f, normalized * (std::log10(maxFreq) - std::log10(minFreq)) + std::log10(minFreq));
}

float EQCoordinateMapper::dbToY(float db, int height) const
{
    auto heightF = static_cast<float>(height);
    float normalized = (db - minDB) / (maxDB - minDB);
    return heightF * (1.0f - normalized);
}
