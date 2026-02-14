# Architecture

## Directory Structure

```
.
├── Source/
│   ├── DSP/                    # Digital signal processing
│   │   ├── BandProcessor.cpp/h
│   │   ├── ProcessBlockCoordinator.cpp/h
│   │   ├── SpectrumAnalyzer.cpp/h
│   ├── GUI/                    # GUI components
│   │   ├── EQCoordinateMapper.cpp/h  # Coordinate mapping utility
│   │   ├── EQOverlay.cpp/h
│   │   ├── FaderMeter.cpp/h
│   │   ├── PanControl.cpp/h
│   │   ├── SpectrumDisplay.cpp/h
│   ├── PluginEditor.cpp/h      # Main GUI
│   └── PluginProcessor.cpp/h   # Audio processing
├── CMakeLists.txt              # Build configuration
└── compile_commands.json       # clangd symbolic link
```

## Parameters

### Global

- Slope: 12/24/48/96/192 dB/oct (ComboBox)

### Per-band (6 bands)

- Bypass: bool (default: Band2-5 ON)
- Solo: bool
- Mono: bool (stereo to mono)
- Pan: -100 (Left) ~ 0 (Center) ~ +100 (Right)
- Gain: -70dB ~ +6dB (log scale, skew 2.5)
- HighpassFreq: 0Hz ~ 20kHz (default: 0Hz = no filter)
- LowpassFreq: 0Hz ~ 20kHz (default: 20kHz = no filter)

## Filter Structure

Each band has independent Highpass + Lowpass filters:

- Highpass 0Hz: no HP (full range)
- Lowpass 20kHz: no LP (full range)
- Both set: band-pass filter

Legacy crossover layout (reference only):

1. Band 1 (lowest): Input -> Lowpass[0]
2. Band 2-5 (mid): Highpass[N-1] -> Lowpass[N]
3. Band 6 (highest): Input -> Highpass[4]

## Technical Notes

### Mono processing

- Location: Source/PluginProcessor.cpp
- Stereo to mono: (L+R)*0.5 to both channels

### Panning

- Location: Source/PluginProcessor.cpp
- Equal-power panning (cos/sin, sqrt2 compensation)

### Metering

- Location: Source/PluginProcessor.cpp
- Mono: pre-pan level
- Stereo: post-pan L/R levels

### Color scheme

- Band colors: deep blue (Band1, hue 0.65) -> light green (Band6, hue 0.35)
- Background: dark blue (0xff1a1a2e)
- Accent: blue (0xff4a90d9)
