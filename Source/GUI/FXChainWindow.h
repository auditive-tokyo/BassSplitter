#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// FXチェーン編集用の別ウィンドウ
class FXChainWindow : public juce::DocumentWindow
{
public:
    FXChainWindow(const juce::String& bandName, int bandIndex)
        : DocumentWindow("FX Chain - " + bandName,
                         juce::Colour(0xff1a1a2e),
                         DocumentWindow::closeButton | DocumentWindow::minimiseButton)
    {
        juce::ignoreUnused(bandIndex);

        setUsingNativeTitleBar(true);
        setResizable(true, false);

        // 空のコンテンツコンポーネント
        auto* content = new FXChainContent(); // NOSONAR - DocumentWindow owns content.
        setContentOwned(content, true);

        centreWithSize(500, 400);
    }

    void closeButtonPressed() override
    {
        // ウィンドウを閉じるのではなく隠す（再利用のため）
        setVisible(false);
    }

private:
    // ウィンドウ内のコンテンツ
    class FXChainContent : public juce::Component
    {
    public:
        FXChainContent()
        {
            setSize(500, 400);
        }

        void paint(juce::Graphics& g) override
        {
            g.fillAll(juce::Colour(0xff1a1a2e));

            g.setColour(juce::Colour(0xff333344));
            g.drawRect(getLocalBounds().reduced(10), 1);

            g.setColour(juce::Colour(0xff8ec8ff));
            g.setFont(juce::FontOptions(16.0f));
            g.drawText("FX Chain (coming soon...)", getLocalBounds(), juce::Justification::centred);
        }
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FXChainWindow)
};
