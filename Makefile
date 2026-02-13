.PHONY: build run install clean cmake check lint help

help:
	@echo "使用可能なコマンド:"
	@echo "  make build    - プロジェクトをビルド"
	@echo "  make run      - ビルドしてStandalone起動"
	@echo "  make install  - ビルドしてVST3/AUをインストール"
	@echo "  make cmake    - CMakeプロジェクトを再生成"
	@echo "  make check    - コンパイルをチェック（エラーのみ表示）"
	@echo "  make lint     - clang-tidyでコード検査"
	@echo "  make clean    - ビルドディレクトリをクリーン"

build:
	cd build && xcodebuild -scheme "BassSplitter_All" -configuration Debug build

run: build
	open build/BassSplitter_artefacts/Debug/Standalone/BassSplitter.app

install: build
	cp -R build/BassSplitter_artefacts/Debug/VST3/BassSplitter.vst3 ~/Library/Audio/Plug-Ins/VST3/
	cp -R build/BassSplitter_artefacts/Debug/AU/BassSplitter.component ~/Library/Audio/Plug-Ins/Components/
	@echo "✓ プラグインをインストールしました。DAWで再スキャンしてください。"

cmake:
	cd build && cmake .. -G Xcode
	cd build-clangd && cmake ..

clean:
	rm -rf build/* build-clangd/*

check:
	cd build && cmake .. -G Xcode 2>/dev/null && xcodebuild -project BassSplitter.xcodeproj -scheme "BassSplitter_All" -configuration Debug build 2>&1 | grep -E "(error|warning):" || echo "✓ ビルドエラー・ワーニングなし"

lint:
	@if command -v clang-tidy &> /dev/null; then \
		echo "clang-tidy でコード検査..."; \
		clang-tidy -p build Source/**/*.cpp Source/**/*.h -- -I/Volumes/AUDITIVE/development/JUCE/modules 2>&1 | head -50; \
	else \
		echo "clang-tidy がインストールされていません。Homebrewでインストール: brew install llvm"; \
	fi
