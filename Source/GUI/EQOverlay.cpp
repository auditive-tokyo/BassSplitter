#include "EQOverlay.h"

EQOverlay::EQOverlay()
{
    setInterceptsMouseClicks(true, true);
    setWantsKeyboardFocus(true); // キーボード入力を受け取るように設定
}

void EQOverlay::setBandEQFrequencies(int bandIndex, float highpassFreq, float lowpassFreq)
{
    if (bandIndex >= 0 && bandIndex < numBands)
    {
        bandHighpassFreqs[static_cast<size_t>(bandIndex)] = highpassFreq;
        bandLowpassFreqs[static_cast<size_t>(bandIndex)] = lowpassFreq;
        repaint();
    }
}

void EQOverlay::setSlope(int slope)
{
    slopeDB = slope;
    repaint();
}

void EQOverlay::setBandBypassed(int bandIndex, bool bypassed)
{
    if (bandIndex >= 0 && bandIndex < numBands)
    {
        bandBypassed[static_cast<size_t>(bandIndex)] = bypassed;
        repaint();
    }
}

void EQOverlay::setFocusedBand(int bandIndex)
{
    if (bandIndex == focusedBand)
        return; // 既に同じバンドがフォーカス中
    focusedBand = bandIndex;
    repaint();
}

// ---- 座標変換 ----

float EQOverlay::frequencyToX(float freq) const
{
    float normalized = (std::log10(freq) - std::log10(minFreq)) / (std::log10(maxFreq) - std::log10(minFreq));
    return normalized * static_cast<float>(getWidth());
}

float EQOverlay::xToFrequency(float x) const
{
    float normalized = x / static_cast<float>(getWidth());
    return std::pow(10.0f, normalized * (std::log10(maxFreq) - std::log10(minFreq)) + std::log10(minFreq));
}

float EQOverlay::dbToY(float db) const
{
    float height = static_cast<float>(getHeight());
    float normalized = (db - minDB) / (maxDB - minDB);
    return height * (1.0f - normalized);
}

juce::Colour EQOverlay::getBandColour(int bandIndex) const
{
    float hue = 0.65f - (static_cast<float>(bandIndex) / (numBands - 1)) * 0.30f;
    return juce::Colour::fromHSV(hue, 0.75f, 0.95f, 1.0f);
}

// ---- EQ計算 ----

float EQOverlay::getBandGain(int bandIndex, float freq) const
{
    int order = slopeDB / 6;

    float hpFreq = bandHighpassFreqs[static_cast<size_t>(bandIndex)];
    float lpFreq = bandLowpassFreqs[static_cast<size_t>(bandIndex)];

    float gainDB = 0.0f;

    if (hpFreq > 1.0f)
    {
        float ratio = hpFreq / freq;
        float magnitude = 1.0f / std::sqrt(1.0f + std::pow(ratio, 2.0f * order));
        gainDB += 20.0f * std::log10(std::max(magnitude, 0.0001f));
    }

    if (lpFreq < 19999.0f)
    {
        float ratio = freq / lpFreq;
        float magnitude = 1.0f / std::sqrt(1.0f + std::pow(ratio, 2.0f * order));
        gainDB += 20.0f * std::log10(std::max(magnitude, 0.0001f));
    }

    return std::max(gainDB, minDB);
}

// ---- ハンドル検索 ----

void EQOverlay::findNearestHandle(float mouseX, float mouseY, int& outBand, bool& outIsHighpass) const
{
    outBand = -1;
    float minDist = handleHitRadius;

    for (int band = 0; band < numBands; ++band)
    {
        if (bandBypassed[static_cast<size_t>(band)])
            continue;

        // HP ハンドル
        float hpFreq = bandHighpassFreqs[static_cast<size_t>(band)];
        if (hpFreq > 1.0f)
        {
            float hx = frequencyToX(hpFreq);
            float hy = dbToY(getBandGain(band, hpFreq));
            float dist = std::sqrt((mouseX - hx) * (mouseX - hx) + (mouseY - hy) * (mouseY - hy));
            if (dist < minDist)
            {
                minDist = dist;
                outBand = band;
                outIsHighpass = true;
            }
        }

        // LP ハンドル
        float lpFreq = bandLowpassFreqs[static_cast<size_t>(band)];
        if (lpFreq < 19999.0f)
        {
            float lx = frequencyToX(lpFreq);
            float ly = dbToY(getBandGain(band, lpFreq));
            float dist = std::sqrt((mouseX - lx) * (mouseX - lx) + (mouseY - ly) * (mouseY - ly));
            if (dist < minDist)
            {
                minDist = dist;
                outBand = band;
                outIsHighpass = false;
            }
        }
    }
}

int EQOverlay::findBandOnCurve(float mouseX, float mouseY) const
{
    // 各バンドのカーブ上にマウスがあるか判定
    for (int band = 0; band < numBands; ++band)
    {
        if (bandBypassed[static_cast<size_t>(band)])
            continue;

        float freq = xToFrequency(mouseX);
        float gainDB = getBandGain(band, freq);
        float curveY = dbToY(gainDB);

        if (std::abs(mouseY - curveY) < curveHitDistance)
            return band;
    }

    return -1;
}

// ---- ポップアップ領域 ----

juce::Rectangle<float> EQOverlay::getPopupHPButtonBounds() const
{
    if (!popup.isVisible)
        return juce::Rectangle<float>();

    float btnY = popup.displayY + 22.0f;
    float btnWidth = 55.0f;
    float btnHeight = 26.0f;
    float gap = 6.0f;
    float popupWidth = 140.0f;
    float totalBtnWidth = btnWidth * 2.0f + gap;
    float btnStartX = popup.displayX + (popupWidth - totalBtnWidth) * 0.5f;

    return juce::Rectangle<float>(btnStartX, btnY, btnWidth, btnHeight);
}

juce::Rectangle<float> EQOverlay::getPopupLPButtonBounds() const
{
    if (!popup.isVisible)
        return juce::Rectangle<float>();

    float btnY = popup.displayY + 22.0f;
    float btnWidth = 55.0f;
    float btnHeight = 26.0f;
    float gap = 6.0f;
    float popupWidth = 140.0f;
    float totalBtnWidth = btnWidth * 2.0f + gap;
    float btnStartX = popup.displayX + (popupWidth - totalBtnWidth) * 0.5f;

    return juce::Rectangle<float>(btnStartX + btnWidth + gap, btnY, btnWidth, btnHeight);
}

juce::Rectangle<float> EQOverlay::getPopupCloseBounds() const
{
    if (!popup.isVisible)
        return juce::Rectangle<float>();

    float popupWidth = 140.0f;
    float popupHeight = 55.0f;
    return juce::Rectangle<float>(popup.displayX, popup.displayY, popupWidth, popupHeight);
}

// ---- マウス操作 ----

void EQOverlay::mouseDown(const juce::MouseEvent& event)
{
    float mx = static_cast<float>(event.x);
    float my = static_cast<float>(event.y);

    // 入力モード中の左クリック → 入力キャンセル
    if (freqInput.isActive && !event.mods.isPopupMenu())
    {
        freqInput.isActive = false;
        dragState.isDragging = false;
        repaint();
        return;
    }

    // ポップアップが表示中ならボタンクリックを処理
    if (popup.isVisible)
    {
        if (popup.canHP && getPopupHPButtonBounds().contains(mx, my))
        {
            // HP ポイントを作成
            float freq = popup.frequency;
            int band = popup.bandIndex;
            bandHighpassFreqs[static_cast<size_t>(band)] = freq;
            if (onEQFrequencyChanged)
                onEQFrequencyChanged(band, true, freq);
            popup.isVisible = false;
            repaint();
            return;
        }
        if (popup.canLP && getPopupLPButtonBounds().contains(mx, my))
        {
            // LP ポイントを作成
            float freq = popup.frequency;
            int band = popup.bandIndex;
            bandLowpassFreqs[static_cast<size_t>(band)] = freq;
            if (onEQFrequencyChanged)
                onEQFrequencyChanged(band, false, freq);
            popup.isVisible = false;
            repaint();
            return;
        }
        if (getPopupCloseBounds().contains(mx, my))
        {
            popup.isVisible = false;
            repaint();
            return;
        }

        // ポップアップ外クリック→閉じる
        popup.isVisible = false;
        repaint();
        // フォールスルーしてハンドル操作を試行
    }

    // 既存ハンドルを検索
    int band = -1;
    bool isHP = true;
    findNearestHandle(mx, my, band, isHP);

    if (band >= 0)
    {
        dragState.isDragging = true;
        dragState.bandIndex = band;
        dragState.isHighpass = isHP;
        setFocusedBand(band); // ハンドルドラッグ時もフォーカスを設定

        // 右クリック → 周波数直接入力モード開始
        if (event.mods.isPopupMenu())
        {
            freqInput.isActive = true;
            freqInput.bandIndex = band;
            freqInput.isHighpass = isHP;
            freqInput.inputText = "";
        }

        repaint();
        return;
    }

    // ハンドルがない場合、focusedBandのカーブ上をクリック → ポップアップ表示
    if (focusedBand >= 0 && !event.mods.isPopupMenu())
    {
        float freq = xToFrequency(mx);
        float gainDB = getBandGain(focusedBand, freq);
        float curveY = dbToY(gainDB);

        if (std::abs(my - curveY) < curveHitDistance)
        {
            // focusedBand のカーブ上をクリック
            popup.bandIndex = focusedBand;
            popup.frequency = freq;
            popup.canHP = (bandHighpassFreqs[static_cast<size_t>(focusedBand)] < 1.5f);
            popup.canLP = (bandLowpassFreqs[static_cast<size_t>(focusedBand)] > 19998.0f);
            popup.isVisible = true;

            // ポップアップ位置を計算
            float popupWidth = 140.0f;
            float popupHeight = 55.0f;
            float px = mx - popupWidth * 0.5f;
            float py = my - popupHeight - 10.0f;

            px = juce::jlimit(2.0f, static_cast<float>(getWidth()) - popupWidth - 2.0f, px);
            if (py < 2.0f)
                py = my + 15.0f;

            popup.displayX = px;
            popup.displayY = py;

            repaint();
            return;
        }
    }
}

void EQOverlay::mouseDrag(const juce::MouseEvent& event)
{
    if (!dragState.isDragging)
        return;

    // 周波数入力モード中はドラッグを無視
    if (freqInput.isActive)
        return;

    float mx = static_cast<float>(event.x);
    float freq = xToFrequency(juce::jlimit(0.0f, static_cast<float>(getWidth()), mx));

    if (dragState.isHighpass)
    {
        float lpFreq = bandLowpassFreqs[static_cast<size_t>(dragState.bandIndex)];
        freq = juce::jlimit(0.0f, lpFreq < 19999.0f ? lpFreq - 1.0f : 20000.0f, freq);
        bandHighpassFreqs[static_cast<size_t>(dragState.bandIndex)] = freq;
    }
    else
    {
        float hpFreq = bandHighpassFreqs[static_cast<size_t>(dragState.bandIndex)];
        freq = juce::jlimit(hpFreq > 1.0f ? hpFreq + 1.0f : 0.0f, 20000.0f, freq);
        bandLowpassFreqs[static_cast<size_t>(dragState.bandIndex)] = freq;
    }

    if (onEQFrequencyChanged)
        onEQFrequencyChanged(dragState.bandIndex, dragState.isHighpass, freq);

    repaint();
}

void EQOverlay::mouseUp(const juce::MouseEvent&)
{
    // 入力モード中は dragState を保持（ツールチップが消えないように）
    if (!freqInput.isActive)
        dragState.isDragging = false;
    repaint();
}

void EQOverlay::mouseMove(const juce::MouseEvent& event)
{
    float mx = static_cast<float>(event.x);
    float my = static_cast<float>(event.y);

    int band = -1;
    bool isHP = true;
    findNearestHandle(mx, my, band, isHP);

    bool changed = (band != hoveredBand) || (isHP != hoveredIsHighpass);
    hoveredBand = band;
    hoveredIsHighpass = isHP;

    // ハンドル上 → リサイズカーソル、カーブ上 → ハンドカーソル
    if (band >= 0)
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    else if (findBandOnCurve(mx, my) >= 0)
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    else
        setMouseCursor(juce::MouseCursor::NormalCursor);

    if (changed)
        repaint();
}

void EQOverlay::mouseDoubleClick(const juce::MouseEvent& event)
{
    float mx = static_cast<float>(event.x);
    float my = static_cast<float>(event.y);

    // 既存ハンドル上でダブルクリック → ポイント削除（デフォルトに戻す）
    int handleBand = -1;
    bool handleIsHP = true;
    findNearestHandle(mx, my, handleBand, handleIsHP);

    if (handleBand >= 0)
    {
        if (handleIsHP)
        {
            bandHighpassFreqs[static_cast<size_t>(handleBand)] = 0.0f;
            if (onEQFrequencyChanged)
                onEQFrequencyChanged(handleBand, true, 0.0f);
        }
        else
        {
            bandLowpassFreqs[static_cast<size_t>(handleBand)] = 20000.0f;
            if (onEQFrequencyChanged)
                onEQFrequencyChanged(handleBand, false, 20000.0f);
        }
        popup.isVisible = false;
        repaint();
        return;
    }

    // フォーカスされたバンドのカーブ上でのみダブルクリック有効
    int band = findBandOnCurve(mx, my);
    if (band >= 0 && (focusedBand == -1 || focusedBand == band)) // フォーカス無し or このバンドがフォーカス中
    {
        setFocusedBand(band); // フォーカスを設定（既に設定済みなら何もしない）

        float freq = xToFrequency(mx);
        float hpFreq = bandHighpassFreqs[static_cast<size_t>(band)];
        float lpFreq = bandLowpassFreqs[static_cast<size_t>(band)];

        // 既にあるかどうかで選択肢を制限
        bool hasHP = hpFreq > 1.0f;
        bool hasLP = lpFreq < 19999.0f;

        popup.isVisible = true;
        popup.bandIndex = band;
        popup.frequency = freq;
        popup.triggerX = mx; // トリガー座標を保存
        popup.triggerY = my;
        popup.canHP = !hasHP; // HPがまだ無ければ作成可
        popup.canLP = !hasLP; // LPがまだ無ければ作成可

        // ポップアップの最終表示位置を一度だけ計算して保存
        float popupWidth = 140.0f;
        float popupHeight = 55.0f;
        float px = mx - popupWidth * 0.5f;
        float py = my - popupHeight - 10.0f;

        // 画面外に出ないように調整
        px = juce::jlimit(2.0f, static_cast<float>(getWidth()) - popupWidth - 2.0f, px);
        if (py < 2.0f)
            py = my + 15.0f;

        popup.displayX = px; // 計算済み位置を固定保存
        popup.displayY = py;

        // 両方あったらポップアップ不要（ダブルクリック削除で対応）
        if (hasHP && hasLP)
            popup.isVisible = false;

        // 片方だけあったら、もう片方を直接作成（ポップアップ省略）
        if (hasHP && !hasLP)
        {
            bandLowpassFreqs[static_cast<size_t>(band)] = freq;
            if (onEQFrequencyChanged)
                onEQFrequencyChanged(band, false, freq);
            popup.isVisible = false;
        }
        else if (!hasHP && hasLP)
        {
            bandHighpassFreqs[static_cast<size_t>(band)] = freq;
            if (onEQFrequencyChanged)
                onEQFrequencyChanged(band, true, freq);
            popup.isVisible = false;
        }

        repaint();
    }
}

// ---- 描画 ----

void EQOverlay::paint(juce::Graphics& g)
{
    drawFilterCurves(g);
    drawEQHandles(g);
    drawDragTooltip(g);
    drawPopup(g);
}

void EQOverlay::drawFilterCurves(juce::Graphics& g)
{
    float width = static_cast<float>(getWidth());

    for (int band = 0; band < numBands; ++band)
    {
        if (bandBypassed[static_cast<size_t>(band)])
            continue;

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

        float strokeWidth = (dragState.isDragging && dragState.bandIndex == band) ? 3.0f : 2.0f;
        auto colour = getBandColour(band);

        // フォーカス状態によってアルファ調整
        float alpha = 1.0f;
        if (focusedBand >= 0 && focusedBand != band)
        {
            alpha = 0.35f; // 非フォーカス時は薄い
        }

        g.setColour(colour.withAlpha(alpha));
        g.strokePath(bandPath, juce::PathStrokeType(strokeWidth));
    }
}

void EQOverlay::drawEQHandles(juce::Graphics& g)
{
    for (int band = 0; band < numBands; ++band)
    {
        if (bandBypassed[static_cast<size_t>(band)])
            continue;

        auto colour = getBandColour(band);

        // HP ハンドル
        float hpFreq = bandHighpassFreqs[static_cast<size_t>(band)];
        if (hpFreq > 1.0f)
        {
            float hx = frequencyToX(hpFreq);
            float hy = dbToY(getBandGain(band, hpFreq));
            bool isActive = (dragState.isDragging && dragState.bandIndex == band && dragState.isHighpass) ||
                            (hoveredBand == band && hoveredIsHighpass);

            float r = isActive ? handleRadius + 2.0f : handleRadius;
            g.setColour(colour.withAlpha(isActive ? 1.0f : 0.8f));
            g.fillEllipse(hx - r, hy - r, r * 2.0f, r * 2.0f);
            g.setColour(juce::Colours::white.withAlpha(0.9f));
            g.drawEllipse(hx - r, hy - r, r * 2.0f, r * 2.0f, 1.5f);

            // HP ラベル
            g.setFont(9.0f);
            g.setColour(juce::Colours::white);
            g.drawText(
                "HP", static_cast<int>(hx - 8), static_cast<int>(hy - r - 12), 16, 10, juce::Justification::centred);
        }

        // LP ハンドル
        float lpFreq = bandLowpassFreqs[static_cast<size_t>(band)];
        if (lpFreq < 19999.0f)
        {
            float lx = frequencyToX(lpFreq);
            float ly = dbToY(getBandGain(band, lpFreq));
            bool isActive = (dragState.isDragging && dragState.bandIndex == band && !dragState.isHighpass) ||
                            (hoveredBand == band && !hoveredIsHighpass);

            float r = isActive ? handleRadius + 2.0f : handleRadius;
            g.setColour(colour.withAlpha(isActive ? 1.0f : 0.8f));
            g.fillEllipse(lx - r, ly - r, r * 2.0f, r * 2.0f);
            g.setColour(juce::Colours::white.withAlpha(0.9f));
            g.drawEllipse(lx - r, ly - r, r * 2.0f, r * 2.0f, 1.5f);

            // LP ラベル
            g.setFont(9.0f);
            g.setColour(juce::Colours::white);
            g.drawText(
                "LP", static_cast<int>(lx - 8), static_cast<int>(ly - r - 12), 16, 10, juce::Justification::centred);
        }
    }
}

void EQOverlay::drawDragTooltip(juce::Graphics& g)
{
    if (!dragState.isDragging)
        return;

    float freq = dragState.isHighpass ? bandHighpassFreqs[static_cast<size_t>(dragState.bandIndex)]
                                      : bandLowpassFreqs[static_cast<size_t>(dragState.bandIndex)];

    juce::String text;
    if (freq >= 1000.0f)
        text = juce::String(freq / 1000.0f, 2) + " kHz";
    else
        text = juce::String(static_cast<int>(freq)) + " Hz";

    text += dragState.isHighpass ? " HP" : " LP";

    float hx = frequencyToX(freq);
    float hy = dbToY(getBandGain(dragState.bandIndex, freq));

    int textWidth = 80;
    int textHeight = 18;
    float tx = hx - textWidth * 0.5f;
    float ty = hy - handleRadius - textHeight - 6.0f;

    tx = juce::jlimit(2.0f, static_cast<float>(getWidth()) - textWidth - 2.0f, tx);
    if (ty < 2.0f)
        ty = hy + handleRadius + 6.0f;

    g.setColour(juce::Colour(0xe0202030));
    g.fillRoundedRectangle(tx, ty, static_cast<float>(textWidth), static_cast<float>(textHeight), 4.0f);
    g.setColour(getBandColour(dragState.bandIndex));
    g.drawRoundedRectangle(tx, ty, static_cast<float>(textWidth), static_cast<float>(textHeight), 4.0f, 1.0f);

    g.setColour(juce::Colours::white);
    g.setFont(12.0f);

    // 入力モード中の表示
    if (freqInput.isActive && freqInput.bandIndex == dragState.bandIndex &&
        freqInput.isHighpass == dragState.isHighpass)
    {
        // テキスト入力フィールドを描画
        g.drawText(freqInput.inputText + "_",
                   static_cast<int>(tx),
                   static_cast<int>(ty),
                   textWidth,
                   textHeight,
                   juce::Justification::centred);
    }
    else
    {
        g.drawText(
            text, static_cast<int>(tx), static_cast<int>(ty), textWidth, textHeight, juce::Justification::centred);
    }
}

void EQOverlay::drawPopup(juce::Graphics& g)
{
    if (!popup.isVisible)
        return;

    auto colour = getBandColour(popup.bandIndex);

    // 周波数テキスト
    juce::String freqText;
    if (popup.frequency >= 1000.0f)
        freqText = juce::String(popup.frequency / 1000.0f, 1) + " kHz";
    else
        freqText = juce::String(static_cast<int>(popup.frequency)) + " Hz";

    // ポップアップ全体の背景（保存された位置から描画）
    float popupWidth = 140.0f;
    float popupHeight = 55.0f;
    float px = popup.displayX; // 計算済み位置を使用（毎フレーム固定）
    float py = popup.displayY;

    g.setColour(juce::Colour(0xf0181828));
    g.fillRoundedRectangle(px, py, popupWidth, popupHeight, 6.0f);
    g.setColour(colour);
    g.drawRoundedRectangle(px, py, popupWidth, popupHeight, 6.0f, 1.5f);

    // 周波数表示
    g.setColour(juce::Colours::white);
    g.setFont(11.0f);
    g.drawText(freqText,
               static_cast<int>(px),
               static_cast<int>(py + 3),
               static_cast<int>(popupWidth),
               16,
               juce::Justification::centred);

    // HP / LP ボタン（ポップアップ相対位置に再計算）
    float btnY = py + 22.0f;
    float btnWidth = 55.0f;
    float btnHeight = 26.0f;
    float gap = 6.0f;
    float totalBtnWidth = btnWidth * 2.0f + gap;
    float btnStartX = px + (popupWidth - totalBtnWidth) * 0.5f;

    // HP ボタン
    auto hpBounds = juce::Rectangle<float>(btnStartX, btnY, btnWidth, btnHeight);
    if (popup.canHP)
    {
        g.setColour(colour.withAlpha(0.3f));
        g.fillRoundedRectangle(hpBounds, 4.0f);
        g.setColour(colour);
        g.drawRoundedRectangle(hpBounds, 4.0f, 1.0f);
        g.setColour(juce::Colours::white);
    }
    else
    {
        g.setColour(juce::Colour(0x40666666));
        g.fillRoundedRectangle(hpBounds, 4.0f);
        g.setColour(juce::Colour(0x80666666));
    }
    g.setFont(12.0f);
    g.drawText("HP", hpBounds.toNearestInt(), juce::Justification::centred);

    // LP ボタン
    auto lpBounds = juce::Rectangle<float>(btnStartX + btnWidth + gap, btnY, btnWidth, btnHeight);
    if (popup.canLP)
    {
        g.setColour(colour.withAlpha(0.3f));
        g.fillRoundedRectangle(lpBounds, 4.0f);
        g.setColour(colour);
        g.drawRoundedRectangle(lpBounds, 4.0f, 1.0f);
        g.setColour(juce::Colours::white);
    }
    else
    {
        g.setColour(juce::Colour(0x40666666));
        g.fillRoundedRectangle(lpBounds, 4.0f);
        g.setColour(juce::Colour(0x80666666));
    }
    g.setFont(12.0f);
    g.drawText("LP", lpBounds.toNearestInt(), juce::Justification::centred);
}

bool EQOverlay::keyPressed(const juce::KeyPress& key)
{
    if (!freqInput.isActive)
        return false;

    // Escape → 入力キャンセル
    if (key.isKeyCode(juce::KeyPress::escapeKey))
    {
        freqInput.isActive = false;
        dragState.isDragging = false;
        repaint();
        return true;
    }

    // Enter → 入力確定
    if (key.isKeyCode(juce::KeyPress::returnKey))
    {
        if (juce::String text = freqInput.inputText.trim(); text.isNotEmpty())
        {
            float freq = 0.0f;

            // テキストを大文字に統一
            juce::String upper = text.toUpperCase();

            // "K" (kHz の k) を含む → kHz に変換
            if (upper.contains("K"))
            {
                // "KHZ" と "K" を削除
                text = text.removeCharacters("kKhHzZ").trim();
                freq = text.getFloatValue() * 1000.0f;
            }
            else
            {
                // Hz を削除（単に数値のみ）
                text = text.removeCharacters("hHzZ").trim();
                freq = text.getFloatValue();
            }

            // 周波数の範囲をクリップ
            freq = juce::jlimit(minFreq, maxFreq, freq);

            // 周波数を設定
            int band = freqInput.bandIndex;
            bool isHP = freqInput.isHighpass;

            if (isHP)
            {
                bandHighpassFreqs[static_cast<size_t>(band)] = freq;
            }
            else
            {
                bandLowpassFreqs[static_cast<size_t>(band)] = freq;
            }

            if (onEQFrequencyChanged)
                onEQFrequencyChanged(band, isHP, freq);
        }

        freqInput.isActive = false;
        dragState.isDragging = false;
        repaint();
        return true;
    }

    // Backspace / Delete → テキスト削除
    if (key.isKeyCode(juce::KeyPress::backspaceKey) || key.isKeyCode(juce::KeyPress::deleteKey))
    {
        if (freqInput.inputText.isNotEmpty())
            freqInput.inputText = freqInput.inputText.dropLastCharacters(1);
        repaint();
        return true;
    }

    // 数字・小数点・単位文字を許可
    auto ch = key.getTextCharacter();
    if (juce::CharacterFunctions::isDigit(ch) || ch == '.' || ch == 'k' || ch == 'K' || ch == 'h' || ch == 'H' ||
        ch == 'z' || ch == 'Z')
    {
        freqInput.inputText += ch;
        repaint();
        return true;
    }

    return false;
}
