/*
 * References:
 * ImGui: https://github.com/ocornut/imgui/wiki/Getting-Started
 * ImGui: https://skia.googlesource.com/external/github.com/ocornut/imgui/+/refs/heads/master/imgui.h
 */

#include "SignalGui.h"
#include "CircularBuffer.h"
#include <iostream>
#include <cmath>
#include <random>

// Constructor
SignalGui::SignalGui()
	: time_buffer_(N_SAMPLES)
	, signal_buffer_(N_SAMPLES)
	, freq_buffer_(N_FREQ)
	, magnitude_buffer_(N_FREQ)
	, psd_buffer_(N_FREQ)
	, current_time_(0.0f)
	, sample_rate_(1000.0f)
	, signal_freq_(50.0f)
	, spectrogram_row_(0)
	, update_counter_(0) {
	std::cout << "Signal generator (SIM) initialized" << std::endl;
	for (int t = 0; t < N_TIME_BINS; ++t) {
		for (int f = 0; f < N_FREQ; ++f) {
			spectrogram_data[t][f] = -80.0f;
		}
	}
}

// Destructor
SignalGui::~SignalGui() {
}

void SignalGui::GenerateNewSamples() {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::normal_distribution<float> noise(0.0f, 0.1f);
	const int batch_size = 64;

	for (int i = 0; i < batch_size; ++i) {
		float dt = 1.0f / sample_rate_;
		float signal = 0.5f * std::sin(2.0f * M_PI * signal_freq_ * current_time_) +
		               0.3f * std::sin(2.0f * M_PI * 150.0f * current_time_) +
		               noise(gen);
		signal_buffer_.Push(signal);
		current_time_ += dt;
	}

	// Update frequency domain every 8 frames
	if (update_counter_ % 8 == 0) {
		for (int i = 0; i < N_FREQ; ++i) {
			float freq = (float)i * sample_rate_ / (2.0f * N_FREQ);
			freq_buffer_.Push(freq);
			float mag = -60.0f;
			if (std::abs(freq - signal_freq_) < 5.0f) mag = -10.0f;
			if (std::abs(freq - 150.0f) < 5.0f) mag = -15.0f;
			mag += 5.0f * (float(rand()) / RAND_MAX - 0.5f);
			magnitude_buffer_.Push(mag);
			psd_buffer_.Push(mag - 10.0f);
		}
	}

	// Update waterfall every 16 frames
	if (update_counter_ % 16 == 0) {
		for (int f = 0; f < N_FREQ; ++f) {
			float freq = (float)f * sample_rate_ / (2.0f * N_FREQ);
			float intensity = -80.0f;  // Blue background

			float varying_freq = signal_freq_ + 15.0f * std::sin(current_time_ * 0.3f);
			if (std::abs(freq - varying_freq) < 8.0f) {
				intensity = -20.0f;  // Strong signal (yellow/red)
			}
			if (std::abs(freq - 150.0f) < 5.0f) {
				intensity = -30.0f;  // Medium signal (orange)
			}

			intensity += 5.0f * (float(rand()) / RAND_MAX - 0.5f);
			spectrogram_data[spectrogram_row_][f] = intensity;
		}
		spectrogram_row_ = (spectrogram_row_ + 1) % N_TIME_BINS;
	}
}

void SignalGui::UpdatePlotData() {
	float dt = 1.0f / sample_rate_;
	for (int i = 0; i < N_SAMPLES; ++i) {
		time_data[i] = current_time_ - (N_SAMPLES - 1 - i) * dt;
	}
    signal_buffer_.CopyLatest(signal_data, N_SAMPLES);
	freq_buffer_.CopyLatest(freq_data, N_FREQ);
	magnitude_buffer_.CopyLatest(magnitude_data, N_FREQ);
	psd_buffer_.CopyLatest(psd_data, N_FREQ);
}


void SignalGui::Update() {
	update_counter_++;
	GenerateNewSamples();
	if (update_counter_ % 4 == 0) {
		UpdatePlotData();
	}

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
    if (ImPlot::BeginPlot("##TimePlot", ImVec2(-1, -1))) {
        ImPlot::SetupAxes("Time [s]", "Amplitude");
        ImPlot::SetupAxesLimits(0.0, 1.0, -1.5, 1.5, ImGuiCond_Always);
        float rel_time[N_SAMPLES];
        for (int i = 0; i < N_SAMPLES; ++i) {
            rel_time[i] = float(i) / sample_rate_;
        }
        ImPlot::PlotLine("Signal", rel_time, signal_data, N_SAMPLES);
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
    ImGui::Text("Waterfall");
    if (ImPlot::BeginPlot("##SpectrogramPlot", ImVec2(-1, -1))) {
        ImPlot::SetupAxes("Frequency [Hz]", "Time", ImPlotAxisFlags_None, ImPlotAxisFlags_Invert);
        ImPlot::SetupAxesLimits(0, sample_rate_/2.0, 0, N_TIME_BINS);

        ImPlot::PlotHeatmap("##Waterfall",
                           (float*)spectrogram_data,
                           N_TIME_BINS, N_FREQ,
                           -80.0, -10.0,
                           nullptr,
                           ImPlotPoint(0, 0),
                           ImPlotPoint(sample_rate_/2.0, N_TIME_BINS));
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
