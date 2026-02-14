#include "PanControl.h"

#include <cmath>

PanControl::PanControl()
{
    // Set range: -50 to +50
    panSlider.setRange(MIN_PAN_VALUE, MAX_PAN_VALUE, 1.0f);
    panSlider.setValue(0.0f, juce::dontSendNotification);
    // Keep slider invisible - we only use it for APVTS attachment

    // スライダーの値が変わったらラベルを更新
    panSlider.onValueChange = [this]() { updateLabelText(); };

    // Pan値入力用ラベル（FaderMeterのdbValueLabelと同じスタイル）
    panValueLabel.setFont(juce::FontOptions(12.0f));
    panValueLabel.setJustificationType(juce::Justification::centred);
    panValueLabel.setColour(juce::Label::textColourId, juce::Colour(0xff4a90d9));
    panValueLabel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    panValueLabel.setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    panValueLabel.setColour(juce::Label::textWhenEditingColourId, juce::Colours::white);
    panValueLabel.setColour(juce::Label::backgroundWhenEditingColourId, juce::Colour(0xff1a1a2e));
    panValueLabel.setColour(juce::Label::outlineWhenEditingColourId, juce::Colour(0xff4a90d9));
    panValueLabel.setEditable(true, true, false); // シングル/ダブルクリックで編集
    panValueLabel.setText("C", juce::dontSendNotification);
    panValueLabel.addListener(this);
    addAndMakeVisible(panValueLabel);
}

void PanControl::configure(juce::Colour colour)
{
    bandColour = colour;
    repaint();
}

void PanControl::attachToParameter(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId)
{
    panAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, panSlider);
}

void PanControl::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // ノブ表示エリア（上部: 70%）
    const auto knobAreaHeight = bounds.getHeight() * 0.70f;
    const auto knobBounds = bounds.removeFromTop(knobAreaHeight);

    const auto centreX = knobBounds.getCentreX();
    const auto centreY = knobBounds.getCentreY();
    const auto radius = juce::jmin(knobBounds.getWidth(), knobBounds.getHeight()) * 0.40f;

    // Background circle
    g.setColour(juce::Colour(0xff1a1a2e));
    g.fillEllipse(knobBounds.withSizeKeepingCentre(radius * 2.4f, radius * 2.4f));
    g.setColour(juce::Colour(0xff333344));
    g.drawEllipse(knobBounds.withSizeKeepingCentre(radius * 2.4f, radius * 2.4f), 1.5f);

    // Knob body
    g.setColour(bandColour);
    g.fillEllipse(knobBounds.withSizeKeepingCentre(radius * 1.9f, radius * 1.9f));

    // Indicator line (shows current position)
    const auto currentValue = getCurrentValue();
    const auto angle = getAngleFromValue(currentValue);
    const auto lineRadius = radius * 0.7f;
    const auto lineEndX = centreX + lineRadius * std::sin(angle);
    const auto lineEndY = centreY - lineRadius * std::cos(angle); // negative because Y is inverted in graphics
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.drawLine(centreX, centreY, lineEndX, lineEndY, 2.5f);

    // Center dot
    g.setColour(juce::Colours::white);
    g.fillEllipse(centreX - 3.0f, centreY - 3.0f, 6.0f, 6.0f);

    // テキスト表示はpanValueLabelが担当するため、ここでは描画しない
}

void PanControl::resized()
{
    auto bounds = getLocalBounds();
    // 下部30%にラベルを配置
    auto labelHeight = bounds.getHeight() * 30 / 100;
    panValueLabel.setBounds(bounds.removeFromBottom(labelHeight));
}

void PanControl::mouseDown(const juce::MouseEvent& event)
{
    isDragging = true;
    lastMouseY = static_cast<float>(event.getPosition().y);
}

void PanControl::mouseDrag(const juce::MouseEvent& event)
{
    if (!isDragging)
        return;

    const auto currentMouseY = static_cast<float>(event.getPosition().y);
    const auto deltaY = lastMouseY - currentMouseY; // negative when dragging up
    const auto sensitivity = 0.5f;                  // pixels to value scale

    auto newValue = getCurrentValue() + (deltaY * sensitivity);
    newValue = juce::jlimit(MIN_PAN_VALUE, MAX_PAN_VALUE, newValue);

    panSlider.setValue(newValue, juce::sendNotificationSync);
    repaint();

    lastMouseY = currentMouseY;
}

void PanControl::mouseUp(const juce::MouseEvent& /* event */)
{
    isDragging = false;
}

void PanControl::mouseDoubleClick(const juce::MouseEvent& /* event */)
{
    panSlider.setValue(0.0f, juce::sendNotificationSync);
    repaint();
}

float PanControl::getCurrentValue() const
{
    return static_cast<float>(panSlider.getValue());
}

juce::String PanControl::formatDisplayValue(float value) const
{
    if (value < -0.5f)
        return juce::String(static_cast<int>(-value)) + "L";
    if (value > 0.5f)
        return juce::String(static_cast<int>(value)) + "R";
    return "C";
}

float PanControl::getAngleFromValue(float value) const
{
    // Map value from [-50, +50] to angle [MIN_ANGLE, MAX_ANGLE]
    const auto normalizedValue = (value - MIN_PAN_VALUE) / (MAX_PAN_VALUE - MIN_PAN_VALUE); // [0, 1]
    return std::lerp(MIN_ANGLE, MAX_ANGLE, normalizedValue);
}

void PanControl::updateLabelText()
{
    panValueLabel.setText(formatDisplayValue(getCurrentValue()), juce::dontSendNotification);
    repaint();
}

void PanControl::editorShown(juce::Label*, juce::TextEditor& editor)
{
    // 数値、マイナス記号、c/C のみ許可
    editor.setInputRestrictions(0, "-0123456789cC");
}

void PanControl::labelTextChanged(juce::Label* labelThatHasChanged)
{
    if (labelThatHasChanged == &panValueLabel)
    {
        juce::String text = panValueLabel.getText().trim();

        // "c" or "C" → center (0)
        if (text.equalsIgnoreCase("c"))
        {
            panSlider.setValue(0.0f, juce::sendNotificationSync);
            return;
        }

        // 数値をパース
        float value = text.getFloatValue();
        value = juce::jlimit(MIN_PAN_VALUE, MAX_PAN_VALUE, value);
        panSlider.setValue(value, juce::sendNotificationSync);
    }
}
