#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * StyleHelper - 編集可能ラベルの共通スタイルを提供するユーティリティ
 *
 * チャンネル名、Pan値、フェーダー値など、編集可能なラベルに
 * 統一されたスタイルを適用するためのヘルパー関数を提供。
 */
namespace StyleHelper
{
// 共通カラー定義
inline constexpr juce::uint32 LABEL_TEXT_COLOUR = 0xff8ec8ff;
inline constexpr juce::uint32 EDITING_BG_COLOUR = 0xff1a1a2e;

/**
 * 編集可能ラベルに共通スタイルを適用する。
 *
 * 通常時: 青テキスト、透明背景
 * 編集時: 白テキスト、暗色背景、青アウトライン
 *
 * @param label 対象のラベル
 */
inline void applyEditableLabelStyle(juce::Label& label)
{
    label.setFont(juce::FontOptions(16.0f));
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(LABEL_TEXT_COLOUR));
    label.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    label.setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    label.setColour(juce::Label::textWhenEditingColourId, juce::Colours::white);
    label.setColour(juce::Label::backgroundWhenEditingColourId, juce::Colour(EDITING_BG_COLOUR));
    label.setColour(juce::Label::outlineWhenEditingColourId, juce::Colour(LABEL_TEXT_COLOUR));
    label.setEditable(true, true, false);
}

} // namespace StyleHelper
