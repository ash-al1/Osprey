# State-of-the-art

## Constellation schemes of interest

Commonly found modulations in the 2.4GHz and 5GHz frequency ranges for typical
wifi, bluetooth, zigbee devices include  BPSK, QPSK, (16, 64, 256, 1024)-QAM,
OQPSK, GFSK.

## Methodology

**Spectrogram-based approach**
+ Frequency / time plot gives spectrogram, more useful than raw IQ in real world
+ Wideband: Reveal where an active burst is present to detect signal presence
+ Constellation mod can be predicted given an active signal

**Training optimization strategies**
+ Focal loss to weigh non-frequent samples however during training a balanced
  dataset can be generated.
+ Adam optimizer as a starter optimizer but look into different regularization
  as L1/L2 differs for radio signals and may have large change.
+ Transforms through torchsig for transfer learning needs (offset in SNR, freq,
  phase, time). Size reduction of dataset by  using lower quantization
  (32->8 bit floats) may result in low accuracy loss. *SIMD loves 8-bit ops!*
+ Knowledge distillation for edge inference

## Implementation, tools, frameworks

**ImGui/ImPlot integration** enables sophisticated RF visualization with
real-time spectrogram display, interactive constellation diagrams, and
GPU-accelerated rendering. The immediate-mode GUI paradigm suits dynamic RF
applications requiring responsive user interfaces.

**C++/USRP integration leverages mature libraries:**
+ UHD library for USRP object handling (receive samples, streamer)
+ Cyclic buffers (lock-free) for continuous streaming
+ Multi-threading handling USRP object, receiving samples, ImGui, buffer, etc.
+ Liquid-DSP for spectrogram generation with SIMD optimization
+ SIMD vectorization for signal processing
+ Memory alignment and prefetching
+ Torchscript the pytorch model for c++ deployment

## Training data and performance thresholds

**TorchSig represents the gold standard** for synthetic data generation,
offering infinite scalability with expertly chosen impairments including AWGN,
multipath fading, frequency offsets, and hardware nonlinearities. The framework
addresses RadioML dataset quality issues while providing 10x larger scale and
4x longer observation windows.

**Critical SNR performance thresholds** define operational boundaries: above 10
dB SNR enables 95-99% accuracy across most approaches, 0-10 dB SNR yields 70-95%
accuracy requiring advanced architectures, while below 0 dB SNR shows severe
degradation demanding specialized noise-robust techniques.

**Real-time performance achievements** NVIDIA small computer deployment (ORIN,
Jetson)
