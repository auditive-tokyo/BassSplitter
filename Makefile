.PHONY: build run install clean cmake help

help:
	@echo "使用可能なコマンド:"
	@echo "  make build    - プロジェクトをビルド"
	@echo "  make run      - ビルドしてStandalone起動"
	@echo "  make install  - ビルドしてVST3/AUをインストール"
	@echo "  make cmake    - CMakeプロジェクトを再生成"
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
