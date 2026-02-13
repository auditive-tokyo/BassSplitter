# BassSplitter - Agent Documentation

## プロジェクト概要

6バンドマルチバンドスプリッター。各バンドに独立したゲイン、パン、モノ/ステレオ切り替え機能を搭載。

## ディレクトリ構造

```
.
├── Source/
│   ├── DSP/                    # デジタル信号処理
│   │   ├── SpectrumAnalyzer.cpp/h
│   ├── GUI/                    # GUIコンポーネント
│   │   ├── EQPanel.cpp/h
│   │   ├── FaderMeter.cpp/h
│   │   ├── SpectrumDisplay.cpp/h
│   ├── PluginEditor.cpp/h      # メインGUI
│   └── PluginProcessor.cpp/h   # オーディオ処理
├── CMakeLists.txt              # ビルド設定
└── compile_commands.json       # clangd用シンボリックリンク
```

## ファイル別機能説明

### メインコンポーネント

#### `PluginProcessor.cpp/h`

- **役割**: オーディオ処理のメイン実装
- **主要機能**:
  - 6バンド per-band EQフィルター (Linkwitz-Riley, 12/24/48/96/192 dB/oct)
  - 各バンドのパラメータ管理 (Gain, Pan, Bypass, Solo, Mono, HP/LP Freq)
  - ステレオ→モノ変換 (`(L+R)*0.5`)
  - 等パワーパンニング (cos/sin法)
  - ステレオピークレベル計算 (メーター用)
- **処理フロー**: EQフィルタリング → モノ処理 → ピーク計算(Mono時) → パン処理 → ピーク計算(Stereo時)
- **スレッドセーフ**: `std::atomic<float>` で L/R ピークレベルを管理

#### `PluginEditor.cpp/h`

- **役割**: GUI全体のレイアウトと管理
- **主要機能**:
  - 6バンドコントロールの配置 (ネーム、EQボタン、パン、フェーダー、ボタン)
  - EQパネルオーバーレイ (スペクトラム上に表示)
  - スペクトラムディスプレイの統合
  - 60Hzタイマーでメーター更新
- **レイアウト (上→下)**:
  - タイトル
  - スペクトラムディスプレイ
  - スロープ選択 / ピークリセット
  - 6バンドコントロール (均等配置)
- **各バンドレイアウト (上→下)**:
  - バンド名ラベル (編集可能)
  - EQボタン (クリックでEQパネル表示)
  - パンスライダー
  - フェーダー+メーター
  - Mono / Solo / Bypass ボタン (縦並び)

### DSP モジュール

#### `DSP/SpectrumAnalyzer.cpp/h`

- **役割**: FFT処理と周波数スペクトラム解析
- **主要機能**:
  - 2048ポイントFFT (JUCE dsp::FFT使用)
  - ウィンドウ関数: Hann窓
  - マグニチュード → dB変換
  - スムージング処理
- **更新**: `pushSamples()` で入力 → `processFFT()` で解析

### GUI コンポーネント

#### `GUI/EQPanel.cpp/h`

- **役割**: 各バンドのEQ設定用オーバーレイパネル
- **主要機能**:
  - 6バンド選択タブ（ラジオボタン）
  - 各バンドのハイパス/ローパススライダー
  - バンドカラー連動（SpectrumDisplayと同一）
  - スペクトラム上にオーバーレイ表示
- **表示/非表示**: 各バンドのEQボタンでトグル
- **レンジ**: 0Hz ~ 20kHz（対数スケール、2000Hz中点）

#### `GUI/FaderMeter.cpp/h`

- **役割**: Abletonスタイルのフェーダー+メーター一体型コンポーネント
- **主要機能**:
  - ステレオメーター表示 (L/R 2バー)
  - モノメーター表示 (1バー)
  - ピークホールド機能
  - 編集可能なdB値ラベル
  - バイパス時のグレーアウト
- **dBレンジ**: -70dB ~ +6dB
- **メーター色**: グリーン → イエロー → レッド (グラデーション)

#### `GUI/SpectrumDisplay.cpp/h`

- **役割**: リアルタイムスペクトラム + EQカーブ表示
- **主要機能**:
  - 対数スケールの周波数軸 (20Hz - 20kHz)
  - スペクトラムのグラデーション描画
  - 6バンドのEQカーブ (バンド色対応)
  - バンドバイパス時は対応カーブ非表示
- **更新**: 30Hzタイマーで再描画

## パラメータ構造

### グローバル

- **Slope**: 12/24/48/96/192 dB/oct (ComboBox)

### 各バンド (6つ)

- **Bypass**: bool (デフォルト: Band2-5のみON)
- **Solo**: bool
- **Mono**: bool (ステレオ→モノ変換)
- **Pan**: -100 (Left) ~ 0 (Center) ~ +100 (Right)
- **Gain**: -70dB ~ +6dB (対数スケール、スキュー2.5)
- **HighpassFreq**: 0Hz ~ 20kHz (デフォルト: 0Hz = フィルターなし)
- **LowpassFreq**: 0Hz ~ 20kHz (デフォルト: 20kHz = フィルターなし)

## フィルター構造

各バンドは独立した **Highpass + Lowpass** EQ フィルターを持ちます：

- Highpass が 0Hz → ハイパスなし（フルレンジ）
- Lowpass が 20kHz → ローパスなし（フルレンジ）
- 両方設定 → バンドパスフィルター

従来のクロスオーバー方式から、各バンドが完全に独立した周波数設定を持つ EQ 方式に変更されました。

## 旧バンド構成（参考）

**注意**: 以下は参考情報です。現在は per-band EQ 方式を使用しています。

1. **Band 1** (最低域): 入力 → Lowpass[0]
2. **Band 2-5** (中域): Highpass[N-1] → Lowpass[N]
3. **Band 6** (最高域): 入力 → Highpass[4]

## 技術的詳細

### モノ処理

- **実装箇所**: [Source/PluginProcessor.cpp](Source/PluginProcessor.cpp)
- **処理**: ステレオ→モノ変換 `(L+R)*0.5` を両チャンネルに出力

### パン処理

- **実装箇所**: [Source/PluginProcessor.cpp](Source/PluginProcessor.cpp)
- **方式**: 等パワーパンニング (cos/sin法、√2補正)

### メーター表示ロジック

- **実装箇所**: [Source/PluginProcessor.cpp](Source/PluginProcessor.cpp)
- **Mono時**: パン処理**前**のレベル (パンに影響されない)
- **Stereo時**: パン処理**後**のL/Rレベル (実際の出力レベル)

### カラースキーム

- **バンド色**: 深い青 (Band1, hue 0.65) → ライトグリーン (Band6, hue 0.35)
- **背景**: ダークブルー系 (0xff1a1a2e)
- **アクセント**: ブルー (0xff4a90d9)

## ビルドシステム

- **CMake**: JUCE 7.x / 8.x 対応
- **ターゲット**: VST3, AU, Standalone
- **GPU レンダリング**: OpenGL (エディタ全体)
- **最適化**: ScopedNoDenormals で非正規化数を無効化

### ビルドコマンド

プロジェクトルートの [Makefile](Makefile) で開発用コマンドを提供しています。

#### 使用可能なコマンド

```bash
make build      # ビルド
make run        # ビルド → Standalone起動
make install    # ビルド → VST3/AUインストール
make cmake      # CMakeプロジェクト再生成（ファイル追加/削除時）
make check      # コンパイルエラーチェック
make lint       # 基本的なコード検査（推奨）
make clean      # ビルドディレクトリをクリーン
```

#### 初回セットアップ

```bash
# ビルドディレクトリ作成
mkdir -p build build-clangd

# CMake生成
make cmake
```

## 開発メモ

### 実装済み機能

- ✅ 各バンド独立のEQフィルター（Highpass/Lowpass）
- ✅ EQパネルUIとスペクトラム上のカーブ表示
- ✅ クロスオーバー方式からper-band EQ方式への移行

### 将来的な機能候補

- Mid/Side処理モード
- L/R単独抽出モード
- プリセット管理
- Qコントロール付きパラメトリックEQ
- AbletonのNative Pluginや3rd party製のVST3/AU Pluginをマウントできるようにする
- JUCE UnitTestの導入（DSP/パラメータテスト）
- SonarQubeのCI統合（静的解析の自動化）

### コード品質

- **行数**: PluginProcessor ~570行、PluginEditor ~300行
- **分割**: DSP/GUI で適切に分離済み
- **推奨**: 800-1000行を超えたら更なる分割を検討

## 参考資料

- **JUCE Framework**: https://juce.com/
- **Linkwitz-Riley Filter**: 4次バターワースフィルターのカスケード
- **等パワーパンニング**: -3dB at center, 一定の音響パワーを保つ

---

## このドキュメントの更新について

### 更新が必要なタイミング

以下の変更があった場合、このAGENT.mdを必ず更新してください：

1. **ファイル構造の変更**
   - 新規ファイルの追加
   - ファイルの削除
   - ファイルの移動やリネーム
   - ディレクトリ構造の変更

2. **アーキテクチャの変更**
   - 主要なクラスや関数の追加・削除
   - 処理フローの大幅な変更
   - DSP アルゴリズムの変更

3. **パラメータの変更**
   - 新パラメータの追加
   - パラメータの削除
   - デフォルト値やレンジの大幅な変更

4. **機能の追加・削除**
   - 新機能の実装
   - 既存機能の削除
   - UIレイアウトの大幅な変更

### 更新手順

```bash
# 1. ディレクトリ構造を確認（プロジェクトルートで実行）
tree -I 'build|build-clangd|.cache' -L 4 --dirsfirst

# 2. AGENT.mdを開いて該当セクションを更新

# 3. 変更内容を確認
git diff AGENT.md
```

### 更新プロンプト例

```
プロジェクトに以下の変更がありました：
- [変更内容を簡潔に記述]

AGENT.mdの以下のセクションを更新してください：
- [更新が必要なセクション名]
```

**注意**: このドキュメントは開発の指針となる重要なファイルです。常に最新の状態に保ってください。
