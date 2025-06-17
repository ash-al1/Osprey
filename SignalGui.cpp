/*
 * References:
 * ImGui: https://github.com/ocornut/imgui/wiki/Getting-Started
 * ImGui: https://skia.googlesource.com/external/github.com/ocornut/imgui/+/refs/heads/master/imgui.h
 */

#include "SignalGui.h"
#include <iostream>
#include <cmath>

// Constructor
SignalGui::SignalGui() {
	GenerateTestData();
}

// Destructor
SignalGui::~SignalGui() {
}

void SignalGui::GenerateTestData() {
	const float fs = 1000.0f;
	const float f1 = 50.0f;
	const float f2 = 150.0f;

	for ( int i = 0; i < N_SAMPLES; i++ ) {
		time_data[i] = (float)i / fs;
		signal_data[i] = 0.5f * std::sin(2.0f * M_PI * f1 * time_data[i]) + 
                        0.3f * std::sin(2.0f * M_PI * f2 * time_data[i]) +
                        0.1f * ((float)rand() / RAND_MAX - 0.5f);
	}

	for ( int i = 0; i < N_FREQ; i++ ) {
		freq_data[i] = (float)i * fs / (2.0f * N_FREQ - 1);

		float mag = -60.0f;
		if (std::abs(freq_data[i] - f1) < 5.0f) mag = -10.0f;
		if (std::abs(freq_data[i] - f2) < 5.0f) mag = -15.0f;

		magnitude_data[i] = mag + 5.0f * ((float)rand() / RAND_MAX - 0.5f);
	}

	for ( int i = 0; i < N_FREQ; i++ ) {
		float psd = -70.0f;
		float f_dist_1 = std::abs(freq_data[i] - f1);
		float f_dist_2 = std::abs(freq_data[i] - f2);

		if ( f_dist_1 < 15.0f )
			psd += 50.0f * std::exp(-f_dist_1 * f_dist_1 / 50.0f);

		if ( f_dist_2 < 20.0f )
			psd += 45.0f * std::exp(-f_dist_2 * f_dist_2 / 80.0f);

		if ( freq_data[i] < 30.0f )
			psd += 10.0f * std::exp(-freq_data[i] / 10.0f);

		psd_data[i] = psd + 2.0f * ((float)rand() / RAND_MAX - 0.5f);
	}


	for ( int t = 0; t < N_TIME_BINS; t++ ) {
		for ( int f = 0; f < N_FREQ; f++ ) {
			int idx = f * N_TIME_BINS + t;

			float freq_hz = (float)f * fs / (2.0f * N_FREQ);
			float time_sec = (float)t / 10.0f;

			float intensity = -60.0f;

			if ( std::abs(freq_hz - (f1 + 20.0f * std::sin(time_sec))) < 10.0f) {
				intensity = -20.0f;
			}
			if ( std::abs(freq_hz - f2) < 5.0f) {
				intensity = -25.0f;
			}
			spectrogram_data[idx] = intensity + 10.0f * ((float)rand() / RAND_MAX - 0.5f);
		}
	}
}

void SignalGui::Update() {
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT), ImGuiCond_Always);


	ImGui::Begin("SigProc", nullptr,
			ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize);

	// Framerate
	ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);

	// Dimensions of each plot
	float quarter_width  = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
	float quarter_height = (ImGui::GetContentRegionAvail().y - ImGui::GetStyle().ItemSpacing.y) * 0.5f;

	// Top Left
	ImGui::BeginChild("TimeDomain", ImVec2(quarter_width, quarter_height), true);
	RenderTimeDomainPlot();
	ImGui::EndChild();
	ImGui::SameLine();

	// Top Right
	ImGui::BeginChild("Frequency", ImVec2(quarter_width, quarter_height), true);
	RenderFrequencyPlot();
	ImGui::EndChild();

	// Bottom Left
	ImGui::BeginChild("Spectrogram", ImVec2(quarter_width, quarter_height), true);
	RenderSpectrogramPlot();
	ImGui::EndChild();
	ImGui::SameLine();

	// Bottom Right
	ImGui::BeginChild("PSD", ImVec2(quarter_width, quarter_height), true);
	RenderReservedSection();
	ImGui::EndChild();
	ImGui::End();
}

void SignalGui::RenderTimeDomainPlot() {
	ImGui::Text("Time domain");
	if ( ImPlot::BeginPlot("##TimePlot", ImVec2(-1, -1))) {
		ImPlot::SetupAxes("Time [s]", "Amplitude");
		ImPlot::SetupAxesLimits(0, 1, -1.5, 1.5);
		ImPlot::PlotLine("Signal", time_data, signal_data, N_SAMPLES);
		ImPlot::EndPlot();
	}
}

void SignalGui::RenderFrequencyPlot() {
	ImGui::Text("Frequency domain");
	if ( ImPlot::BeginPlot("##FreqPlot", ImVec2(-1, -1))) {
		ImPlot::SetupAxes("Frequency [Hz]", "Magnitude [dB]");
		ImPlot::SetupAxesLimits(0, 250, -80, 0);
		ImPlot::PlotLine("Magnitude", freq_data, magnitude_data, N_FREQ);
		ImPlot::EndPlot();
	}
}

void SignalGui::RenderSpectrogramPlot() {
	ImGui::Text("Spectrogram");
	if ( ImPlot::BeginPlot("##SpectrogramPlot", ImVec2(-1, -1))) {
		ImPlot::SetupAxes("Time [S]", "Frequency [KHz]", ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
		ImPlot::SetupAxesLimits(0, 10, 0, 250);
		ImPlot::PlotHeatmap("###Spectrogram", spectrogram_data, N_FREQ, N_TIME_BINS,
				-70, -10 , nullptr, ImPlotPoint(0, 0), ImPlotPoint(10, 250));
		ImPlot::EndPlot();
	}
}

void SignalGui::RenderReservedSection() {
	ImGui::Text("Power spectral density");
	if ( ImPlot::BeginPlot("##PSDPlot", ImVec2(-1, -1))) {
		ImPlot::SetupAxes("Frequency [Hz]", "PSD [dB/Hz]");
		ImPlot::SetupAxesLimits(0, 250, -90, -20);
		ImPlot::PlotLine("PSD", freq_data, psd_data, N_FREQ);
		ImPlot::EndPlot();
	}
}
