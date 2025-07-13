#pragma once

#include "imgui.h"
#include "implot.h"
#include "CircularBuffer.h"
#include "SDRDevice.h"
#include "FFTProcessor.h"
#include <memory>
#include <atomic>
#include <string>

class SignalGui {
private:
    static constexpr int WINDOW_WIDTH  = 1200;
    static constexpr int WINDOW_HEIGHT = 800;

    static constexpr int N_SAMPLES = 1000;
    static constexpr int N_FREQ = 512;
    static constexpr int N_TIME_BINS = 100;

    // Plot buffers
    CircularBuffer<float> time_buffer_;
    CircularBuffer<float> signal_buffer_;
    CircularBuffer<float> freq_buffer_;
    CircularBuffer<float> magnitude_buffer_;
    CircularBuffer<float> psd_buffer_;

    int spectrogram_row_;

    // Plot displays
    float time_data[N_SAMPLES];
    float signal_data[N_SAMPLES];
    float freq_data[N_FREQ];
    float magnitude_data[N_FREQ];
    float psd_data[N_FREQ];
    float spectrogram_data[N_TIME_BINS][N_FREQ];

    float current_time_;
    float sample_rate_;
    int update_counter_;

    std::unique_ptr<SDRDevice> sdr_device_;
    SDRConfig device_config_;
    std::atomic<size_t> samples_received_;
    std::atomic<size_t> overflow_count_;

	std::unique_ptr<SpectrumAnalyzer> spectrum_analyzer_;
    bool spectrum_ready_ = false;

public:
    SignalGui();
    ~SignalGui();

    bool Initialize(const SDRConfig& config);
    
    // Main update function called each frame
    void Update();

    // Control
    bool StartReceiving();
    void StopReceiving();
    bool IsReceiving() const;
    
    // Params
    bool SetFrequency(double freq_hz);
    bool SetSampleRate(double rate_sps);
    bool SetGain(double gain_db);
    bool SetBandwidth(double bandwidth_hz);
    
    // Device info
    std::string GetDeviceType() const;
    std::string GetDeviceInfo() const;
    SDRStatus GetDeviceStatus() const;
    SDRCapabilities GetDeviceCapabilities() const;

private:
    void ProcessSamples(const std::complex<float>* samples, size_t count);
    
    // Update functions
    void UpdatePlotData();
    void UpdateFrequencyDomain();
    void UpdateWaterfall();
    
    // Rendering functions
    void RenderTimeDomainPlot();
    void RenderFrequencyPlot();
    void RenderSpectrogramPlot();
    void RenderPowerSpectralDensity();
    void RenderStatusBar();
    
    // FFT placeholder (implement with a library like FFTW or kissfft)
	void ComputeFFT(const float* time_domain, float* freq_domain, float* magnitude, size_t fft_size);
};
