#include "LevelMeter.h"

LevelMeter::LevelMeter()
{
    startTimerHz(60); // 60fps更新
}

LevelMeter::~LevelMeter()
{
    stopTimer();
}

void LevelMeter::setLevel(float newLevel)
{
    // リニア値をdBに変換して正規化
    float db = linearToDB(newLevel);
    targetLevel = dbToNormalized(db);

    // ピークホールドを更新（より高いピークのみ）
    if (targetLevel > peakHoldLevel)
    {
        peakHoldLevel = targetLevel;
        peakHoldDB = db;
    }
}

void LevelMeter::resetPeakHold()
{
    peakHoldLevel = 0.0f;
    peakHoldDB = minDB;
}

void LevelMeter::setMeterColour(juce::Colour colour)
{
    meterColour = colour;
}

void LevelMeter::setBypassed(bool bypassed)
{
    isBypassed = bypassed;
    if (bypassed)
    {
        targetLevel = 0.0f;
    }
}

void LevelMeter::timerCallback()
{
    // スムーズな減衰/アタック
    if (targetLevel > currentLevel)
    {
        // アタック（高速）
        currentLevel = currentLevel * attackCoeff + targetLevel * (1.0f - attackCoeff);
    }
    else
    {
        // リリース（減衰）
        currentLevel = currentLevel * releaseCoeff + targetLevel * (1.0f - releaseCoeff);
    }

    // 微小値はゼロにクランプ
    if (currentLevel < 0.001f)
        currentLevel = 0.0f;

    repaint();
}

float LevelMeter::linearToDB(float linear)
{
    if (linear <= 0.0f)
        return -100.0f;
    return 20.0f * std::log10(linear);
}

float LevelMeter::dbToNormalized(float db) const
{
    if (db <= minDB)
        return 0.0f;
    if (db >= maxDB)
        return 1.0f;
    return (db - minDB) / (maxDB - minDB);
}

void LevelMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float width = bounds.getWidth();

    // 背景
    g.setColour(juce::Colour(0xff1a1a2e));
    g.fillRoundedRectangle(bounds, 2.0f);

    if (isBypassed)
    {
        // バイパス時はグレーアウト
        g.setColour(juce::Colour(0xff333344));
        g.fillRoundedRectangle(bounds.reduced(1.0f), 2.0f);
        return;
    }

    // メーター領域
    auto meterBounds = bounds.reduced(2.0f);
    float meterHeight = meterBounds.getHeight();
    float meterWidth = meterBounds.getWidth();

    // メーターの高さを計算（下から上へ）
    float levelHeight = currentLevel * meterHeight;

    if (levelHeight > 0.0f)
    {
        // グラデーション（下：緑 → 中：黄色 → 上：赤）
        juce::ColourGradient gradient(juce::Colour(0xff00cc66), // 緑（下）
                                      meterBounds.getX(),
                                      meterBounds.getBottom(),
                                      juce::Colour(0xffff3333), // 赤（上）
                                      meterBounds.getX(),
                                      meterBounds.getY(),
                                      false);
        gradient.addColour(0.6, juce::Colour(0xffffcc00));  // 黄色（-6dB付近）
        gradient.addColour(0.85, juce::Colour(0xffff6633)); // オレンジ（0dB付近）

        g.setGradientFill(gradient);

        // メーターを描画（下から上へ）
        g.fillRoundedRectangle(
            meterBounds.getX(), meterBounds.getBottom() - levelHeight, meterWidth, levelHeight, 1.0f);
    }

    // ピークホールドライン
    if (peakHoldLevel > 0.0f)
    {
        float peakY = meterBounds.getBottom() - (peakHoldLevel * meterHeight);

        // ピークホールドの色（0dB以上は赤、それ以下は白）
        if (peakHoldDB >= 0.0f)
            g.setColour(juce::Colour(0xffff3333));
        else
            g.setColour(juce::Colours::white);

        g.fillRect(meterBounds.getX(), peakY - 1.0f, meterWidth, 2.0f);

        // ピークdB値を表示
        juce::String peakText;
        if (peakHoldDB <= -60.0f)
            peakText = "-inf";
        else
            peakText = juce::String(peakHoldDB, 1);
        
        g.setFont(8.0f);
        float textY = peakY - 10.0f;
        if (textY < meterBounds.getY())
            textY = peakY + 3.0f;  // 上に余裕がなければ下に表示
        
        g.drawText(peakText, 
                   juce::Rectangle<float>(meterBounds.getX() - 2.0f, textY, meterWidth + 4.0f, 10.0f),
                   juce::Justification::centred, false);
    }

    // dB目盛りライン（オプション：幅が十分にある場合）
    if (width > 15.0f)
    {
        g.setColour(juce::Colour(0x40ffffff));
        std::array<float, 4> dbMarks = {0.0f, -6.0f, -12.0f, -24.0f};
        for (float db : dbMarks)
        {
            float normalized = dbToNormalized(db);
            float y = meterBounds.getBottom() - (normalized * meterHeight);
            g.drawHorizontalLine(static_cast<int>(y), meterBounds.getX(), meterBounds.getRight());
        }
    }

    // 枠線
    g.setColour(juce::Colour(0xff4a4a5a));
    g.drawRoundedRectangle(bounds, 2.0f, 1.0f);
}
