# GUI Components

## PluginEditor.cpp/h

- Role: GUI layout and orchestration
- Responsibilities:
  - Layout for 6-band controls
  - SpectrumDisplay integration (with EQOverlay)
  - EQOverlay callback to APVTS
  - 60Hz timer for meter updates
- Layout (top to bottom):
  - Title
  - SpectrumDisplay (with EQOverlay)
  - Slope selection / Peak reset
  - 6-band controls (equal width)
- Band layout (top to bottom):
  - Band name label (editable)
  - Pan slider
  - Fader + meter
  - Mono / Solo / Bypass buttons (vertical)

## GUI/EQCoordinateMapper.cpp/h

- Role: Coordinate conversion utility for EQ visualization
- Responsibilities:
  - frequencyToX(): frequency (Hz) -> x (log scale)
  - xToFrequency(): x -> frequency (Hz)
  - dbToY(): dB -> y (linear scale -48dB to +6dB)
- Design: Stateless, reusable for future EQ components

## GUI/EQOverlay.cpp/h

- Role: Inline EQ editing overlay (FabFilter-style)
- Responsibilities:
  - EQ curve rendering
  - Draggable HP/LP handles
  - Double-click to create/remove HP/LP points (focused band)
  - HP/LP selection popup
  - Tooltip during drag
  - Right-click during drag to enter frequency text input
  - Band focus via EQ button or handle drag
- Callback: onEQFrequencyChanged -> PluginEditor
- Range: 20Hz to 20kHz (log scale)

## GUI/FaderMeter.cpp/h

- Role: Combined fader + meter
- Responsibilities:
  - Stereo meter (L/R) and mono meter
  - Peak hold
  - Editable dB label
  - Bypass greying
- dB range: -70dB to +6dB
- Meter color: green -> yellow -> red (gradient)

## GUI/SpectrumDisplay.cpp/h

- Role: Real-time spectrum display (FFT only)
- Responsibilities:
  - Log frequency axis (20Hz to 20kHz)
  - Spectrum gradient rendering
  - Grid and labels
  - Hosts EQOverlay as child
- Update: 30Hz timer
- Accessor: getEQOverlay()
