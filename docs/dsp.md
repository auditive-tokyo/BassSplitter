# DSP Modules

## DSP/BandProcessor.cpp/h

- Role: Per-band DSP processing
- Responsibilities:
  - Apply HP/LP EQ filters
  - Mono conversion and panning
  - L/R peak calculation and storage
- Design: processSingleBand() orchestrates private const helpers

## DSP/ProcessBlockCoordinator.cpp/h

- Role: processBlock orchestration (parameters, buffers, mix)
- Responsibilities:
  - Slope to filter stage conversion
  - Update EQ frequencies per band
  - Load band parameters
  - Mix output and clear bypassed peaks
- Design: Stateless, const methods

## DSP/SpectrumAnalyzer.cpp/h

- Role: FFT-based spectrum analysis
- Responsibilities:
  - 2048-point FFT (JUCE dsp::FFT)
  - Hann window
  - Magnitude to dB conversion
  - Smoothing
- Flow: pushSamples() -> processFFT()
