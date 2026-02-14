#include "SpectrumDisplay.h"

SpectrumDisplay::SpectrumDisplay(SpectrumAnalyzer& analyzerRef) : analyzer(analyzerRef)
{
    addAndMakeVisible(eqOverlay);
}

SpectrumDisplay::~SpectrumDisplay()
{
    stopTimer();
}

void SpectrumDisplay::visibilityChanged()
{
    if (isVisible())
        startTimerHz(30);
    else
        stopTimer();
}

void SpectrumDisplay::timerCallback()
{
    if (analyzer.isNextBlockReady())
    {
        analyzer.processFFT();
        repaint();
    }
}

// ---- 座標変換 ----

float SpectrumDisplay::frequencyToX(float freq) const
{
    float normalized = (std::log10(freq) - std::log10(minFreq)) / (std::log10(maxFreq) - std::log10(minFreq));
    return normalized * static_cast<float>(getWidth());
}

// ---- 描画 ----

void SpectrumDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float width = bounds.getWidth();
    float height = bounds.getHeight();

    // 背景
    g.fillAll(juce::Colour(0xff0d0d1a));

    // グリッド線（周波数）
    g.setColour(juce::Colour(0xff2a2a3a));
    std::array<float, 10> gridFreqs = {
        20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f};
    for (float freq : gridFreqs)
    {
        float x = frequencyToX(freq);
        g.drawVerticalLine(static_cast<int>(x), 0.0f, height);
    }

    // 周波数ラベル
    g.setColour(juce::Colour(0xff666677));
    g.setFont(10.0f);
    std::array<std::pair<float, const char*>, 10> labels = {{{20.0f, "20"},
                                                             {50.0f, "50"},
                                                             {100.0f, "100"},
                                                             {200.0f, "200"},
                                                             {500.0f, "500"},
                                                             {1000.0f, "1k"},
                                                             {2000.0f, "2k"},
                                                             {5000.0f, "5k"},
                                                             {10000.0f, "10k"},
                                                             {20000.0f, "20k"}}};
    for (const auto& [freq, label] : labels)
    {
        float x = frequencyToX(freq);
        g.drawText(
            label, static_cast<int>(x) - 15, static_cast<int>(height) - 15, 30, 12, juce::Justification::centred);
    }

    // スペクトラムを描画
    const auto& spectrumData = analyzer.getSpectrumData();
    double sampleRate = analyzer.getSampleRate();
    int numBins = SpectrumAnalyzer::fftSize / 2;

    juce::Path spectrumPath;
    bool pathStarted = false;

    for (int i = 1; i < numBins; ++i)
    {
        float binFreq =
            static_cast<float>(i) * static_cast<float>(sampleRate) / static_cast<float>(SpectrumAnalyzer::fftSize);

        if (binFreq < minFreq || binFreq > maxFreq)
            continue;

        float x = frequencyToX(binFreq);
        float y = height - (spectrumData[static_cast<size_t>(i)] * height * 0.9f);

        if (!pathStarted)
        {
            spectrumPath.startNewSubPath(x, y);
            pathStarted = true;
        }
        else
        {
            spectrumPath.lineTo(x, y);
        }
    }

    // スペクトラムの塗りつぶし
    if (pathStarted)
    {
        juce::Path fillPath = spectrumPath;
        fillPath.lineTo(width, height);
        fillPath.lineTo(0, height);
        fillPath.closeSubPath();

        g.setGradientFill(
            juce::ColourGradient(juce::Colour(0x804a90d9), 0, 0, juce::Colour(0x204a90d9), 0, height, false));
        g.fillPath(fillPath);

        g.setColour(juce::Colour(0xff4a90d9));
        g.strokePath(spectrumPath, juce::PathStrokeType(1.5f));
    }

    // 0dBライン（参照線）
    float dbNormalized = (0.0f - minDB) / (maxDB - minDB);
    float zeroDBY = height * (1.0f - dbNormalized);
    g.setColour(juce::Colour(0x40ffffff));
    g.drawHorizontalLine(static_cast<int>(zeroDBY), 0.0f, width);

    // 枠線
    g.setColour(juce::Colour(0xff4a4a5a));
    g.drawRect(bounds, 1.0f);
}

void SpectrumDisplay::resized()
{
    // EQOverlay はスペクトラム全体に重ねる
    eqOverlay.setBounds(getLocalBounds());
}
