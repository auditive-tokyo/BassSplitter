#include "FaderMeter.h"

#include <cmath>

FaderMeter::FaderMeter()
{
    // 内部スライダーの設定（非表示、Attachment用）
    internalSlider.setSliderStyle(juce::Slider::LinearVertical);
    internalSlider.setRange(minDB, maxDB, 0.01);
    internalSlider.setValue(0.0);
    internalSlider.setDoubleClickReturnValue(true, 0.0);
    internalSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    addAndMakeVisible(internalSlider);
    internalSlider.setAlpha(0.0f); // 完全に透明（描画は自前で行う）

    // スライダーの値が変わったら再描画とラベル更新
    internalSlider.onValueChange = [this]()
    {
        auto db = static_cast<float>(internalSlider.getValue());
        juce::String text;
        if (db <= -69.5f)
            text = juce::String(juce::CharPointer_UTF8("-\xe2\x88\x9e"));
        else
            text = juce::String(db, 1);
        dbValueLabel.setText(text, juce::dontSendNotification);
        repaint();
    };

    // dB値入力用ラベル
    dbValueLabel.setFont(juce::FontOptions(16.0f));
    dbValueLabel.setJustificationType(juce::Justification::centred);
    dbValueLabel.setColour(juce::Label::textColourId, faderColour);
    dbValueLabel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    dbValueLabel.setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    dbValueLabel.setColour(juce::Label::textWhenEditingColourId, juce::Colours::white);
    dbValueLabel.setColour(juce::Label::backgroundWhenEditingColourId, juce::Colour(0xff1a1a2e));
    dbValueLabel.setColour(juce::Label::outlineWhenEditingColourId, faderColour);
    dbValueLabel.setEditable(true, true, false);
    dbValueLabel.setText("0.0", juce::dontSendNotification);
    dbValueLabel.addListener(this);
    addAndMakeVisible(dbValueLabel);

    startTimerHz(60);
}

FaderMeter::~FaderMeter()
{
    stopTimer();
}

void FaderMeter::setLevel(float leftLevel, float rightLevel)
{
    float dbL = linearToDB(leftLevel);
    float dbR = linearToDB(rightLevel);
    targetLevelL = dbToNormalized(dbL);
    targetLevelR = dbToNormalized(dbR);

    // ピークホールドは左右の最大値
    float maxLevel = std::max(targetLevelL, targetLevelR);
    float maxDb = std::max(dbL, dbR);
    if (maxLevel > peakHoldLevel)
    {
        peakHoldLevel = maxLevel;
        peakHoldDB = maxDb;
    }
}

void FaderMeter::setLevel(float newLevel)
{
    // モノラル用：左右同じ値
    setLevel(newLevel, newLevel);
}

void FaderMeter::setMono(bool mono)
{
    isMono = mono;
}

void FaderMeter::resetPeakHold()
{
    peakHoldLevel = 0.0f;
    peakHoldDB = minDB;
}

void FaderMeter::setFaderColour(juce::Colour colour)
{
    faderColour = colour;
}

void FaderMeter::setBypassed(bool bypassed)
{
    isBypassed = bypassed;
    if (bypassed)
    {
        targetLevelL = 0.0f;
        targetLevelR = 0.0f;
    }
}

void FaderMeter::timerCallback()
{
    // 左チャンネル
    if (targetLevelL > currentLevelL)
        currentLevelL = currentLevelL * attackCoeff + targetLevelL * (1.0f - attackCoeff);
    else
        currentLevelL = currentLevelL * releaseCoeff + targetLevelL * (1.0f - releaseCoeff);
    if (currentLevelL < 0.001f)
        currentLevelL = 0.0f;

    // 右チャンネル
    if (targetLevelR > currentLevelR)
        currentLevelR = currentLevelR * attackCoeff + targetLevelR * (1.0f - attackCoeff);
    else
        currentLevelR = currentLevelR * releaseCoeff + targetLevelR * (1.0f - releaseCoeff);
    if (currentLevelR < 0.001f)
        currentLevelR = 0.0f;

    repaint();
}

float FaderMeter::linearToDB(float linear)
{
    if (linear <= 0.0f)
        return -100.0f;
    return 20.0f * std::log10(linear);
}

float FaderMeter::dbToNormalized(float db) const
{
    if (db <= minDB)
        return 0.0f;
    if (db >= maxDB)
        return 1.0f;
    return (db - minDB) / (maxDB - minDB);
}

float FaderMeter::yToDb(float y, float trackTop, float trackBottom) const
{
    float normalized = 1.0f - (y - trackTop) / (trackBottom - trackTop);
    return std::lerp(minDB, maxDB, normalized);
}

float FaderMeter::dbToY(float db, float trackTop, float trackBottom) const
{
    float normalized = (db - minDB) / (maxDB - minDB);
    return trackBottom - normalized * (trackBottom - trackTop);
}

void FaderMeter::mouseDown(const juce::MouseEvent& e)
{
    if (isBypassed)
        return;

    auto bounds = getLocalBounds().toFloat();
    float trackTop = 25.0f;
    float trackBottom = bounds.getHeight() - 25.0f;

    float db = yToDb(static_cast<float>(e.y), trackTop, trackBottom);
    db = juce::jlimit(minDB, maxDB, db);
    internalSlider.setValue(db, juce::sendNotification);
}

void FaderMeter::mouseDrag(const juce::MouseEvent& e)
{
    if (isBypassed)
        return;

    auto bounds = getLocalBounds().toFloat();
    float trackTop = 25.0f;
    float trackBottom = bounds.getHeight() - 25.0f;

    float db = yToDb(static_cast<float>(e.y), trackTop, trackBottom);
    db = juce::jlimit(minDB, maxDB, db);
    internalSlider.setValue(db, juce::sendNotification);
}

void FaderMeter::mouseDoubleClick(const juce::MouseEvent&)
{
    if (!isBypassed)
        internalSlider.setValue(0.0, juce::sendNotification); // 0dBにリセット
}

void FaderMeter::resized()
{
    internalSlider.setBounds(getLocalBounds());

    // dB値ラベルは下部に配置
    auto bounds = getLocalBounds();
    dbValueLabel.setBounds(bounds.removeFromBottom(20));
}

void FaderMeter::labelTextChanged(juce::Label* labelThatHasChanged)
{
    if (labelThatHasChanged == &dbValueLabel)
    {
        juce::String text = dbValueLabel.getText().trim();

        // -∞ の場合
        if (text.containsIgnoreCase("inf") || text == juce::String(juce::CharPointer_UTF8("-\xe2\x88\x9e")))
        {
            internalSlider.setValue(minDB, juce::sendNotification);
            return;
        }

        // 数値をパース
        float value = text.getFloatValue();
        value = juce::jlimit(minDB, maxDB, value);
        internalSlider.setValue(value, juce::sendNotification);
    }
}

void FaderMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // ピーク値表示エリア（上部）
    float peakDisplayHeight = 20.0f;
    auto peakArea = bounds.removeFromTop(peakDisplayHeight);

    // dB値表示エリア（下部）- Labelが配置されるのでスペースを確保
    bounds.removeFromBottom(20.0f);

    // メイントラック
    auto trackBounds = bounds.reduced(2.0f, 3.0f);
    float trackWidth = trackBounds.getWidth();

    // 背景（トラック）
    g.setColour(juce::Colour(0xff0d0d1a));
    g.fillRoundedRectangle(trackBounds, 3.0f);

    if (isBypassed)
    {
        g.setColour(juce::Colour(0xff333344));
        g.fillRoundedRectangle(trackBounds.reduced(2.0f), 2.0f);

        // ピーク表示（バイパス時は---）
        g.setColour(juce::Colours::grey);
        g.setFont(11.0f);
        g.drawText("---", peakArea, juce::Justification::centred);

        g.setColour(juce::Colour(0xff4a4a5a));
        g.drawRoundedRectangle(trackBounds, 3.0f, 1.0f);
        return;
    }

    // メーター領域（フェーダーハンドルと同じ幅）
    float handleInset = trackWidth * 0.15f;
    auto meterBounds = trackBounds.reduced(handleInset, 3.0f);
    float meterHeight = meterBounds.getHeight();
    float meterWidth = meterBounds.getWidth();

    // グラデーション設定（フェーダーカラーベース）
    juce::ColourGradient gradient(faderColour.darker(0.6f), // 下（暗め）
                                  meterBounds.getX(),
                                  meterBounds.getBottom(),
                                  faderColour.brighter(0.3f), // 上（明るめ）
                                  meterBounds.getX(),
                                  meterBounds.getY(),
                                  false);
    gradient.addColour(0.5, faderColour); // 中間はそのまま

    if (isMono)
    {
        // モノモード：1本のメーター（中央）
        if (float levelHeight = currentLevelL * meterHeight; levelHeight > 0.0f)
        {
            g.setGradientFill(gradient);
            g.fillRoundedRectangle(
                meterBounds.getX(), meterBounds.getBottom() - levelHeight, meterWidth, levelHeight, 2.0f);
        }
    }
    else
    {
        // ステレオモード：左右2本のメーター
        float meterGap = 2.0f;
        float singleMeterWidth = (meterWidth - meterGap) / 2.0f;

        // 左チャンネル
        if (float levelHeightL = currentLevelL * meterHeight; levelHeightL > 0.0f)
        {
            g.setGradientFill(gradient);
            g.fillRoundedRectangle(
                meterBounds.getX(), meterBounds.getBottom() - levelHeightL, singleMeterWidth, levelHeightL, 2.0f);
        }

        // 右チャンネル
        if (float levelHeightR = currentLevelR * meterHeight; levelHeightR > 0.0f)
        {
            g.setGradientFill(gradient);
            g.fillRoundedRectangle(meterBounds.getX() + singleMeterWidth + meterGap,
                                   meterBounds.getBottom() - levelHeightR,
                                   singleMeterWidth,
                                   levelHeightR,
                                   2.0f);
        }
    }

    // dB目盛り（右側に小さく）
    g.setColour(juce::Colour(0x60ffffff));
    g.setFont(8.0f);
    std::array<float, 6> dbMarks = {0.0f, -6.0f, -12.0f, -24.0f, -48.0f, -70.0f};
    for (float db : dbMarks)
    {
        float normalized = dbToNormalized(db);
        float y = meterBounds.getBottom() - (normalized * meterHeight);
        g.drawHorizontalLine(static_cast<int>(y), meterBounds.getX(), meterBounds.getX() + 4.0f);
        g.drawHorizontalLine(static_cast<int>(y), meterBounds.getRight() - 4.0f, meterBounds.getRight());
    }

    // 0dBライン（強調）
    float zeroDbY = meterBounds.getBottom() - (dbToNormalized(0.0f) * meterHeight);
    g.setColour(juce::Colour(0x80ff6600));
    g.fillRect(meterBounds.getX(), zeroDbY - 0.5f, meterWidth, 1.0f);

    // ピークホールドライン
    if (peakHoldLevel > 0.01f)
    {
        float peakY = meterBounds.getBottom() - (peakHoldLevel * meterHeight);
        g.setColour(peakHoldDB >= 0.0f ? juce::Colour(0xffff3333) : juce::Colours::white);
        g.fillRect(meterBounds.getX(), peakY - 1.0f, meterWidth, 2.0f);
    }

    // フェーダーハンドル
    auto faderDb = static_cast<float>(internalSlider.getValue());
    float faderNormalized = dbToNormalized(faderDb);
    float faderY = meterBounds.getBottom() - (faderNormalized * meterHeight);

    // ハンドル本体（細め）
    float handleHeight = 8.0f;
    float handleY = faderY - handleHeight / 2.0f;

    juce::Rectangle handleRect(
        trackBounds.getX() + handleInset, handleY, trackWidth - handleInset * 2.0f, handleHeight);

    // ハンドル影
    g.setColour(juce::Colour(0x40000000));
    g.fillRoundedRectangle(handleRect.translated(0.0f, 1.0f), 2.0f);

    // ハンドル本体
    g.setColour(faderColour.darker(0.2f));
    g.fillRoundedRectangle(handleRect, 2.0f);

    // ハンドル上部ハイライト
    g.setColour(faderColour.brighter(0.3f));
    g.fillRoundedRectangle(handleRect.removeFromTop(handleHeight * 0.4f), 2.0f);

    // ハンドル中央ライン
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.fillRect(handleRect.getX() + 2.0f, faderY - 0.5f, handleRect.getWidth() - 4.0f, 1.0f);

    // 枠線
    g.setColour(juce::Colour(0xff4a4a5a));
    g.drawRoundedRectangle(trackBounds, 3.0f, 1.0f);

    // ピークdB値表示（上部）
    juce::String peakText;
    if (peakHoldDB <= -69.0f)
        peakText = "-inf";
    else
        peakText = juce::String(peakHoldDB, 2);

    g.setColour(peakHoldDB >= 0.0f ? juce::Colour(0xffff4444) : juce::Colours::white);
    g.setFont(11.0f);
    g.drawText(peakText, peakArea, juce::Justification::centred);

    // 現在のフェーダー値表示はLabelで行う（クリックで編集可能）
    // dbValueLabelが表示するため、ここでは描画しない
}
