# Machine Learning

## SigTorch

Useful for dataset generation, data transforms. Also comes with many pretrained
models for quick testing. This can be used instead of MATLAB to quickly generate
digitally modulated signals (simulated) found in our center frequency of
interest (2.4GHz, 5GHz).

### Dataset Generation

Raw IQ is a tuple of real and imaginary values. These tuples can be used in
numerous ways but the cutting edge method involves deriving FFT, PSD (power
spectral density) and AP (Amplitude-Phase). Torchsig could be used to generate
the raw IQ types for us then manually deriving these values for saving on disk.

### Models

Spatial based (CNN): Window over raw IQ or spectrogram output. Generally a good
baseline for testing and simplest case. VGGNet shown in RadioML to perform well
enough. I have a feeling CNNs will perform better with spectrogram data instead
of raw IQ, that is my intuitive feeling considering CNNs are optimal for
computer vision tasks while LSTMs are better for time varying signals (stock
market and raw IQ signals).

Time varying based (LSTM): Digitall modulated signals are inherently time
varying, intuitively LSTM layers should work well. Even my research generally
shows this to be accurate, we tested on RadioML 2018 but not the Journal paper
which was real signals.

Attention based (Transformers): Accepted to be SOTA (2025) although the compute
and space requirements are fucked. We will build transformer based models for
testing but if the increased accuracy is not worth the training time, we'll
skip.

