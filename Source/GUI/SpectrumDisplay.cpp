#include "SpectrumDisplay.h"

SpectrumDisplay::SpectrumDisplay(SpectrumAnalyzer& analyzerRef)
    : analyzer(analyzerRef)
{
    // タイマーは visibilityChanged で開始
}

SpectrumDisplay::~SpectrumDisplay()
{
    stopTimer();
}

void SpectrumDisplay::visibilityChanged()
{
    // GUIが表示されている時だけタイマーを動かす
    if (isVisible())
        startTimerHz(30);
    else
        stopTimer();
}

void SpectrumDisplay::timerCallback()
{
    // FFTデータが準備できていたら処理して再描画
    if (analyzer.isNextBlockReady())
    {
        analyzer.processFFT();
        repaint();
    }
}

void SpectrumDisplay::setCrossoverFrequency(float freq)
{
    crossoverFrequency = freq;
    repaint();
}

void SpectrumDisplay::setSlope(int slope)
{
    slopeDB = slope;
    repaint();
}

float SpectrumDisplay::frequencyToX(float freq) const
{
    // 対数スケールで周波数をX座標に変換
    float normalized = (std::log10(freq) - std::log10(minFreq)) 
                     / (std::log10(maxFreq) - std::log10(minFreq));
    return normalized * static_cast<float>(getWidth());
}

float SpectrumDisplay::xToFrequency(float x) const
{
    float normalized = x / static_cast<float>(getWidth());
    return std::pow(10.0f, normalized * (std::log10(maxFreq) - std::log10(minFreq)) + std::log10(minFreq));
}

void SpectrumDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float width = bounds.getWidth();
    float height = bounds.getHeight();

    // 背景
    g.fillAll(juce::Colour(0xff0d0d1a));

    // グリッド線（周波数）
    g.setColour(juce::Colour(0xff2a2a3a));
    std::array<float, 12> gridFreqs = { 20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f };
    for (float freq : gridFreqs)
    {
        float x = frequencyToX(freq);
        g.drawVerticalLine(static_cast<int>(x), 0.0f, height);
    }

    // 周波数ラベル
    g.setColour(juce::Colour(0xff666677));
    g.setFont(10.0f);
    std::array<std::pair<float, const char*>, 10> labels = {{
        {20.0f, "20"},
        {50.0f, "50"},
        {100.0f, "100"},
        {200.0f, "200"},
        {500.0f, "500"},
        {1000.0f, "1k"},
        {2000.0f, "2k"},
        {5000.0f, "5k"},
        {10000.0f, "10k"},
        {20000.0f, "20k"}
    }};
    for (auto& [freq, label] : labels)
    {
        float x = frequencyToX(freq);
        g.drawText(label, static_cast<int>(x) - 15, static_cast<int>(height) - 15, 30, 12, juce::Justification::centred);
    }

    // スペクトラムを描画
    const auto& spectrumData = analyzer.getSpectrumData();
    double sampleRate = analyzer.getSampleRate();
    int numBins = SpectrumAnalyzer::fftSize / 2;

    juce::Path spectrumPath;
    bool pathStarted = false;

    for (int i = 1; i < numBins; ++i)
    {
        // FFTビンの周波数を計算
        float binFreq = static_cast<float>(i) * static_cast<float>(sampleRate) / static_cast<float>(SpectrumAnalyzer::fftSize);

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

        // グラデーション塗りつぶし
        g.setGradientFill(juce::ColourGradient(
            juce::Colour(0x804a90d9), 0, 0,
            juce::Colour(0x204a90d9), 0, height,
            false
        ));
        g.fillPath(fillPath);

        // スペクトラムライン
        g.setColour(juce::Colour(0xff4a90d9));
        g.strokePath(spectrumPath, juce::PathStrokeType(1.5f));
    }

    // フィルターカーブを描画
    drawFilterCurve(g, height);

    // クロスオーバーライン
    float crossoverX = frequencyToX(crossoverFrequency);

    // クロスオーバーライン本体（点線）
    g.setColour(juce::Colour(0xaaffffff));
    float dashLengths[] = { 4.0f, 4.0f };
    g.drawDashedLine(juce::Line<float>(crossoverX, 0.0f, crossoverX, height), dashLengths, 2, 1.0f);

    // クロスオーバー周波数ラベル
    g.setColour(juce::Colours::white);
    g.setFont(12.0f);
    juce::String freqText = juce::String(static_cast<int>(crossoverFrequency)) + " Hz";
    g.drawText(freqText, static_cast<int>(crossoverX) - 25, 5, 50, 15, juce::Justification::centred);

    // 枠線
    g.setColour(juce::Colour(0xff4a4a5a));
    g.drawRect(bounds, 1.0f);
}

void SpectrumDisplay::drawFilterCurve(juce::Graphics& g, float height)
{
    float width = static_cast<float>(getWidth());
    
    // dBをY座標に変換するラムダ
    auto dbToY = [height](float db) {
        float normalized = (db - minDB) / (maxDB - minDB);
        return height * (1.0f - normalized);
    };

    // ローパスカーブ（緑）
    juce::Path lowpassPath;
    bool lowStarted = false;
    for (float x = 0; x < width; x += 2.0f)
    {
        float freq = xToFrequency(x);
        float gainDB = getLowpassGain(freq);
        float y = dbToY(gainDB);

        if (!lowStarted)
        {
            lowpassPath.startNewSubPath(x, y);
            lowStarted = true;
        }
        else
        {
            lowpassPath.lineTo(x, y);
        }
    }
    g.setColour(juce::Colour(0xff00cc66));
    g.strokePath(lowpassPath, juce::PathStrokeType(2.0f));

    // ハイパスカーブ（オレンジ）
    juce::Path highpassPath;
    bool highStarted = false;
    for (float x = 0; x < width; x += 2.0f)
    {
        float freq = xToFrequency(x);
        float gainDB = getHighpassGain(freq);
        float y = dbToY(gainDB);

        if (!highStarted)
        {
            highpassPath.startNewSubPath(x, y);
            highStarted = true;
        }
        else
        {
            highpassPath.lineTo(x, y);
        }
    }
    g.setColour(juce::Colour(0xffff9933));
    g.strokePath(highpassPath, juce::PathStrokeType(2.0f));

    // 0dBライン（参照線）
    float zeroDBY = dbToY(0.0f);
    g.setColour(juce::Colour(0x40ffffff));
    g.drawHorizontalLine(static_cast<int>(zeroDBY), 0.0f, width);
}

float SpectrumDisplay::getLowpassGain(float freq) const
{
    // Linkwitz-Rileyローパスフィルターの周波数応答（近似）
    // LR: -6dB at crossover, slope depends on order
    float ratio = freq / crossoverFrequency;
    
    // 次数に応じたスロープ
    // 12dB/oct = 2次, 24dB/oct = 4次, 48dB/oct = 8次
    int order = slopeDB / 6;  // 12->2, 24->4, 48->8
    
    // Butterworth magnitude response: |H(s)| = 1 / sqrt(1 + (f/fc)^(2n))
    // Linkwitz-Rileyはさらに-6dB at fcなので調整
    float magnitude = 1.0f / std::sqrt(1.0f + std::pow(ratio, 2.0f * order));
    
    // dBに変換
    float gainDB = 20.0f * std::log10(std::max(magnitude, 0.0001f));
    return std::max(gainDB, minDB);
}

float SpectrumDisplay::getHighpassGain(float freq) const
{
    // Linkwitz-Rileyハイパスフィルターの周波数応答（近似）
    float ratio = crossoverFrequency / freq;
    
    int order = slopeDB / 6;
    
    float magnitude = 1.0f / std::sqrt(1.0f + std::pow(ratio, 2.0f * order));
    
    float gainDB = 20.0f * std::log10(std::max(magnitude, 0.0001f));
    return std::max(gainDB, minDB);
}

void SpectrumDisplay::resized()
{
    // 特に処理なし
}
