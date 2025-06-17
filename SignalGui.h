#pragma once

#include "imgui.h"
#include "implot.h"

class SignalGui {
private:
	static constexpr int WINDOW_WIDTH  = 1200;
	static constexpr int WINDOW_HEIGHT = 800;

	static constexpr int N_SAMPLES = 1000;
	static constexpr int N_FREQ = 512;
	static constexpr int N_TIME_BINS = 100;

	float time_data[N_SAMPLES];
	float signal_data[N_SAMPLES];
	float freq_data[N_FREQ];
	float magnitude_data[N_FREQ];
	float psd_data[N_FREQ];
	float spectrogram_data[N_SAMPLES * N_TIME_BINS];

public:
	SignalGui();
	~SignalGui();

	void Update();

private:
	void GenerateTestData();
	void RenderTimeDomainPlot();
	void RenderFrequencyPlot();
	void RenderSpectrogramPlot();
	void RenderReservedSection();
};
