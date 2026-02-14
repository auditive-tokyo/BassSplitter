# BassSplitter - Agent Documentation

## プロジェクト概要

6バンドマルチバンドスプリッター。各バンドに独立したゲイン、パン、モノ/ステレオ切り替え機能を搭載。

## プロジェクトルール

**実装完了後の必須チェック**

- 🔴 **ファイル追加・削除・変更後**: `make cmake` で CMakeLists.txt 再生成
- ✅ **すべての実装完了後**: `make lint` でコード品質検査（警告なし）
- ✅ **すべての実装完了後**: `make check` でビルドエラー検査（エラーなし）
- ✅ **両方が PASS したら**: `make run` でスタンドアロン起動テスト
- ✅ **最後に**: AGENT.md を更新（変更内容を反映）

**ビルド成功の条件**

- `make lint` でエラー・ワーニング 0
- `make check` で "✓ ビルドエラー・ワーニングなし" 表示
- `make run` でアプリ起動・基本動作確認（クラッシュなし）

> **使用可能なコマンド**
>
> ```bash
> make build      # ビルド
> make run        # ビルド → Standalone起動
> make launch     # Standalone起動（ビルド済み前提、ビルドなし）
> make install    # ビルド → VST3/AUインストール
> make cmake      # CMakeプロジェクト再生成（ファイル追加/削除時）
> make check      # コンパイルエラーチェック
> make lint       # 基本的なコード検査（推奨）
> make clean      # ビルドディレクトリをクリーン
> ```
>
> **初回セットアップ**
>
> ```bash
> ビルドディレクトリ作成
> mkdir -p build build-clangd
> CMake生成
> make cmake
> ```

## Docs

- Directory structure and architecture: docs/architecture.md
- DSP modules: docs/dsp.md
- GUI components: docs/gui.md

## ビルドシステム

- **CMake**: JUCE 7.x / 8.x 対応
- **ターゲット**: VST3, AU, Standalone
- **GPU レンダリング**: OpenGL (エディタ全体)
- **最適化**: ScopedNoDenormals で非正規化数を無効化

### ビルドコマンド

プロジェクトルートの [Makefile](Makefile) で開発用コマンドを提供しています。

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

### コード品質とリファクタリング

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

**注意**: SonarQube対応（警告修正、リファクタリングなど）については、コード修正のたびに発生する細かい変更のため、AGENT.mdへの記載は不要です。ドキュメントが肥大化して読みづらくなることを避けるため、アーキテクチャに影響する大きな変更のみを記録してください。

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
