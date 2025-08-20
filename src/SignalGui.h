#pragma once

#include <memory>
#include <atomic>
#include <string>
#include <vector>
#include <chrono>

#include "imgui.h"
#include "implot.h"
#include "CircularBuffer.h"
#include "SDRDevice.h"
#include "FFTProcessor.h"
#include "STFTSpectrogram.h"
#include "Spectro3D.h"

class SignalGui {
private:
    static constexpr int WINDOW_WIDTH  = 1920;
    static constexpr int WINDOW_HEIGHT = 1080;

	int fft_size_;
	int num_freq_bins_;

    static constexpr int N_SAMPLES = 1000;
    static constexpr int N_TIME_BINS = 300;

    static constexpr int FREQ_UPDATE_INTERVAL_MS = 25;
    static constexpr int WATERFALL_UPDATE_INTERVAL_MS = 40;

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
	std::vector<float> spectrogram_data;

	std::atomic<bool> new_time_data_available_;
	std::atomic<bool> new_freq_data_available_;

	std::chrono::steady_clock::time_point last_freq_update_time_;
	std::chrono::steady_clock::time_point last_waterfall_update_time_;

	std::vector<float> real_samples_buffer;
	std::vector<float> rel_time_array;
	std::vector<float> time_data_offsets;

    float current_time_;
    float sample_rate_;
    int update_counter_;

	double last_sample_rate_;
	double last_center_freq_;
	bool freq_array_valid_;

    std::unique_ptr<SDRDevice> sdr_device_;
    SDRConfig device_config_;
    std::atomic<size_t> samples_received_;
    std::atomic<size_t> overflow_count_;

    // Updated to use new SpectrogramAnalyzer
    std::unique_ptr<SpectrogramAnalyzer> spectrogram_analyzer_;
    bool spectrum_ready_ = false;

    std::unique_ptr<Spectro3D> waterfall_3d_;

	int custom_spectrum_colormap_ = -1;
	void spectrumColormap();

	std::unique_ptr<STFTSpectrogram> stft_processor_;
	CircularBuffer<std::complex<float>> stft_sample_buffer_;
    std::vector<float> stft_spectrogram_data_;
    std::vector<float> stft_freq_axis_;
    std::vector<float> stft_time_axis_;
    int stft_freq_bins_ = 0;
    int stft_time_frames_ = 0;
    std::atomic<bool> stft_data_ready_{false};
    std::atomic<bool> stft_display_ready_{false};
    std::chrono::steady_clock::time_point last_stft_update_time_;
    static constexpr int STFT_UPDATE_INTERVAL_MS = 500;
    static constexpr int STFT_FFT_SIZE = 1024;
    static constexpr int STFT_FFT_STRIDE = 512;
    static constexpr int MAX_STFT_TIME_FRAMES = 120;
	static constexpr size_t STFT_BUFFER_SIZE = 65536;

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
	void updateRelTimeArray();
	void updateTimeDataOffsets();
	void UpdateSTFTSpectrogram();
    
    // Rendering functions
    void RenderTimeDomainPlot();
    void RenderFrequencyPlot();
    void RenderSpectrogramPlot();
    void RenderSTFTSpectrogram();
    void RenderPowerSpectralDensity();
    void Render3DSpectrogramView();
    void RenderStatusBar();
	void RenderRFMLTab();

	void initializeSTFTProcessor();
};
