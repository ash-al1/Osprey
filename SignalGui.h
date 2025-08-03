#pragma once

#include "imgui.h"
#include "implot.h"
#include "CircularBuffer.h"
#include "SDRDevice.h"
#include "FFTProcessor.h"
#include "Spectro3D.h"
#include <memory>
#include <atomic>
#include <string>
#include <vector>

class SignalGui {
private:
    static constexpr int WINDOW_WIDTH  = 1200;
    static constexpr int WINDOW_HEIGHT = 800;

	int fft_size_;
	int num_freq_bins_;

    static constexpr int N_SAMPLES = 1000;
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
	std::vector<float> freq_data;
	std::vector<float> magnitude_data;
	std::vector<float> psd_data;
	std::vector<std::vector<float>> spectrogram_data;	

    float current_time_;
    float sample_rate_;
    int update_counter_;

    std::unique_ptr<SDRDevice> sdr_device_;
    SDRConfig device_config_;
    std::atomic<size_t> samples_received_;
    std::atomic<size_t> overflow_count_;

    // Updated to use new SpectrogramAnalyzer
    std::unique_ptr<SpectrogramAnalyzer> spectrogram_analyzer_;
    bool spectrum_ready_ = false;

    std::unique_ptr<Spectro3D> waterfall_3d_;

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

	// Tab windows
	void RenderMultiPlotView();
    void Render3DSpectrogramView();

};
