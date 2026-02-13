#include "SpectrumDisplay.h"

SpectrumDisplay::SpectrumDisplay(SpectrumAnalyzer& analyzerRef) : analyzer(analyzerRef)
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

void SpectrumDisplay::setBandEQFrequencies(int bandIndex, float highpassFreq, float lowpassFreq)
{
    if (bandIndex >= 0 && bandIndex < numBands)
    {
        bandHighpassFreqs[static_cast<size_t>(bandIndex)] = highpassFreq;
        bandLowpassFreqs[static_cast<size_t>(bandIndex)] = lowpassFreq;
        repaint();
    }
}

void SpectrumDisplay::setSlope(int slope)
{
    slopeDB = slope;
    repaint();
}

void SpectrumDisplay::setBandBypassed(int bandIndex, bool bypassed)
{
    if (bandIndex >= 0 && bandIndex < numBands)
    {
        bandBypassed[static_cast<size_t>(bandIndex)] = bypassed;
        repaint();
    }
}

float SpectrumDisplay::frequencyToX(float freq) const
{
    // 対数スケールで周波数をX座標に変換
    float normalized = (std::log10(freq) - std::log10(minFreq)) / (std::log10(maxFreq) - std::log10(minFreq));
    return normalized * static_cast<float>(getWidth());
}

float SpectrumDisplay::xToFrequency(float x) const
{
    float normalized = x / static_cast<float>(getWidth());
    return std::pow(10.0f, normalized * (std::log10(maxFreq) - std::log10(minFreq)) + std::log10(minFreq));
}

juce::Colour SpectrumDisplay::getBandColour(int bandIndex) const
{
    // 各バンドに異なる色を割り当て（深い青→ライトグリーンのグラデーション、フェーダーと同じ）
    // Band 1 (index 0) = Deep Blue (hue 0.65), Band 6 (index 5) = Light Green (hue 0.35)
    float hue = 0.65f - (static_cast<float>(bandIndex) / (numBands - 1)) * 0.30f;
    return juce::Colour::fromHSV(hue, 0.75f, 0.95f, 1.0f);
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
    for (auto& [freq, label] : labels)
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
        // FFTビンの周波数を計算
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

        // グラデーション塗りつぶし
        g.setGradientFill(
            juce::ColourGradient(juce::Colour(0x804a90d9), 0, 0, juce::Colour(0x204a90d9), 0, height, false));
        g.fillPath(fillPath);

        // スペクトラムライン
        g.setColour(juce::Colour(0xff4a90d9));
        g.strokePath(spectrumPath, juce::PathStrokeType(1.5f));
    }

    // フィルターカーブを描画（6バンド）
    drawFilterCurves(g, height);

    // 0dBライン（参照線）
    auto dbToY = [height](float db)
    {
        float normalized = (db - minDB) / (maxDB - minDB);
        return height * (1.0f - normalized);
    };
    float zeroDBY = dbToY(0.0f);
    g.setColour(juce::Colour(0x40ffffff));
    g.drawHorizontalLine(static_cast<int>(zeroDBY), 0.0f, width);

    // 枠線
    g.setColour(juce::Colour(0xff4a4a5a));
    g.drawRect(bounds, 1.0f);
}

void SpectrumDisplay::drawFilterCurves(juce::Graphics& g, float height)
{
    float width = static_cast<float>(getWidth());

    // dBをY座標に変換するラムダ
    auto dbToY = [height](float db)
    {
        float normalized = (db - minDB) / (maxDB - minDB);
        return height * (1.0f - normalized);
    };

    // 各バンドのカーブを描画（バイパスでないバンドのみ）
    for (int band = 0; band < numBands; ++band)
    {
        if (bandBypassed[static_cast<size_t>(band)])
            continue; // バイパスされているバンドはスキップ

        juce::Path bandPath;
        bool pathStarted = false;

        for (int i = 0; i < static_cast<int>(width); i += 2)
        {
            float x = static_cast<float>(i);
            float freq = xToFrequency(x);
            float gainDB = getBandGain(band, freq);
            float y = dbToY(gainDB);

            if (!pathStarted)
            {
                bandPath.startNewSubPath(x, y);
                pathStarted = true;
            }
            else
            {
                bandPath.lineTo(x, y);
            }
        }

        // バンドの色で描画
        g.setColour(getBandColour(band));
        g.strokePath(bandPath, juce::PathStrokeType(2.0f));
    }
}

float SpectrumDisplay::getBandGain(int bandIndex, float freq) const
{
    // 各バンドのハイパス・ローパスEQによるゲインを計算
    int order = slopeDB / 6; // 12->2, 24->4, 48->8, 96->16, 192->32

    float hpFreq = bandHighpassFreqs[static_cast<size_t>(bandIndex)];
    float lpFreq = bandLowpassFreqs[static_cast<size_t>(bandIndex)];

    float gainDB = 0.0f;

    // ハイパスフィルター（hpFreq > 0 のときのみ適用）
    if (hpFreq > 1.0f)
    {
        float ratio = hpFreq / freq;
        float magnitude = 1.0f / std::sqrt(1.0f + std::pow(ratio, 2.0f * order));
        gainDB += 20.0f * std::log10(std::max(magnitude, 0.0001f));
    }

    // ローパスフィルター（lpFreq < 20kHz のときのみ適用）
    if (lpFreq < 19999.0f)
    {
        float ratio = freq / lpFreq;
        float magnitude = 1.0f / std::sqrt(1.0f + std::pow(ratio, 2.0f * order));
        gainDB += 20.0f * std::log10(std::max(magnitude, 0.0001f));
    }

    return std::max(gainDB, minDB);
}

void SpectrumDisplay::resized()
{
    // 特に処理なし
}
