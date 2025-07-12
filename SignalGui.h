#pragma once

#include "imgui.h"
#include "implot.h"
#include "CircularBuffer.h"
#include "UsrpController.h"
#include <memory>
#include <atomic>

class SignalGui {
public:
	enum class Mode {
		SIMULATION,
		USRP,
	};

private:
	static constexpr int WINDOW_WIDTH  = 1200;
	static constexpr int WINDOW_HEIGHT = 800;

	static constexpr int N_SAMPLES = 1000;
	static constexpr int N_FREQ = 512;
	static constexpr int N_TIME_BINS = 100;

	CircularBuffer<float> time_buffer_;
	CircularBuffer<float> signal_buffer_;
	CircularBuffer<float> freq_buffer_;
	CircularBuffer<float> magnitude_buffer_;
	CircularBuffer<float> psd_buffer_;

	int spectrogram_row_;

	float time_data[N_SAMPLES];
	float signal_data[N_SAMPLES];
	float freq_data[N_FREQ];
	float magnitude_data[N_FREQ];
	float psd_data[N_FREQ];
	float spectrogram_data[N_TIME_BINS][N_FREQ];

	float current_time_;
	float sample_rate_;
	float signal_freq_;
	int update_counter_;

	Mode mode_;

	std::unique_ptr<UsrpController> usrp_controller_;
	std::atomic<bool> usrp_initialized_;
	std::atomic<size_t> samples_received_;
	std::atomic<size_t> overflow_count_;

	double usrp_frequency_;
	double usrp_sample_rate_;
	double usrp_gain_;

public:
	SignalGui(Mode mode = Mode::SIMULATION);
	~SignalGui();

	void Update();

	bool InitializeUsrp();
	void ShutdownUsrp();
	bool IsUsrpInitialized() const { return usrp_initialized_.load(); }

	void SetUsrpFrequency(double freq_hz) { usrp_frequency_ = freq_hz; }
	void SetUsrpSampleRate(double rate_sps) { usrp_sample_rate_ = rate_sps; }
	void SetUsrpGain(double gain_db) { usrp_gain_ = gain_db; }

	void GetUsrpFrequency() const { return usrp_frequency_; }
	void GetUsrpSampleRate() const { return usrp_sample_rate_; }
	void GetUsrpGain() const { return usrp_gain_; }

private:
	void GenerateSamples();
	void UsrpSamples();
	void UpdatePlotData();
	void RenderTimeDomainPlot();
	void RenderFrequencyPlot();
	void RenderSpectrogramPlot();
	void RenderReservedSection();
	void ProcessUsrpSamples(const std::complex<float>* samples, size_t count);
	void UpdateUsrpFrequencyDomain();
	void UpdateUsrpWaterfall();
};
