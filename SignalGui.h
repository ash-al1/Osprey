#pragma once

#include "imgui.h"
#include "implot.h"
#include "CircularBuffer.h"

class SignalGui {
private:
	static constexpr int WINDOW_WIDTH  = 900;
	static constexpr int WINDOW_HEIGHT = 600;

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

public:
	SignalGui();
	~SignalGui();

	void Update();

private:
	void GenerateNewSamples();
	void UpdatePlotData();
	void RenderTimeDomainPlot();
	void RenderFrequencyPlot();
	void RenderSpectrogramPlot();
	void RenderReservedSection();
};
