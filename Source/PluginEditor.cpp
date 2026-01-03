#include "PluginProcessor.h"
#include "PluginEditor.h"

BassSplitterAudioProcessorEditor::BassSplitterAudioProcessorEditor(BassSplitterAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // タイトルラベル
    titleLabel.setText("BassSplitter", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(24.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    // 情報ラベル
    infoLabel.setText("Crossover: 200 Hz", juce::dontSendNotification);
    infoLabel.setFont(juce::Font(16.0f));
    infoLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(infoLabel);

    setSize(300, 150);
}

BassSplitterAudioProcessorEditor::~BassSplitterAudioProcessorEditor()
{
}

void BassSplitterAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a2e));

    // 装飾線
    g.setColour(juce::Colour(0xff4a90d9));
    g.drawRect(getLocalBounds().reduced(10), 2);
}

void BassSplitterAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);

    titleLabel.setBounds(area.removeFromTop(40));
    area.removeFromTop(10);
    infoLabel.setBounds(area.removeFromTop(30));
}
