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
SignalGui::SignalGui(Mode mode)
	: time_buffer_(N_SAMPLES)
	, signal_buffer_(N_SAMPLES)
	, freq_buffer_(N_FREQ)
	, magnitude_buffer_(N_FREQ)
	, psd_buffer_(N_FREQ)
	, current_time_(0.0f)
	, sample_rate_(1000.0f)
	, signal_freq_(50.0f)
	, spectrogram_row_(0)
	, update_counter_(0)
	, mode_(mode)
	, usrp_controller(nullptr)
	, usrp_initialized_(false)
	, samples_received_(0)
	, overflow_count_(0)
	, usrp_frequency_(2.45e9)
	, usrp_samples_rate_(10e6)
	, usrp_gain_(40.0) {

	if (mode_ == Mode::SIMULATION) {
		std::cout << "Using simulated mode" << std::endl;
	} else {
		std::cout << "Using USRP mode" << std::endl;
		if (InitializeUsrp()) {
			std::cout << "USRP initialized successfully" << std::endl;
		} else {
			std::cout << "Failed to initialize USRP" << std::endl;
		}
	}
	for (int t = 0; t < N_TIME_BINS; ++t) {
		for (int f = 0; f < N_FREQ; ++f) {
			spectrogram_data[t][f] = -80.0f;
		}
	}
}

// Destructor
SignalGui::~SignalGui() {
}

bool SignalGui::InitializeUsrp() {
	if (usrp_initialize_.load()) { return true; }

	try {
		usrp_controller_ = std::make_unique<UsrpController>();
		if (!usrp_controller_->Initialize("32C1EC6")) {
			std::cerr << "Failed to initialize USRP: " << usrp_controller_->GetLastError() << std::endl;
			usrp_controller_.reset();
			return false;
		}
		if (!usrp_controller_->SetRxFrequency(usrp_frequency_)) {
			std::cerr << "Failed to set frequency: " << usrp_controller_->GetLastError() << std::endl;
			return false;
		}
		if (!usrp_controller_->SetRxSampleRate(usrp_sample_rate_)) {
			std::cerr << "Failed to set sample rate: " << usrp_controller_->GetLastError() << std::endl;
			return false;
		}
		if (!usrp_controller_->SetRxGain(usrp_gain_)) {
			std::cerr << "Failed to set gain: " << usrp_controller_->GetLastError() << std::endl;
			return false;
		}
		sample_rate_ = (float) usrp_controller_->GetRxSampleRate();

		std::cout << "Usrp configuration:" << std::endl;
		std::cout << " freq:" << usrp_controller_->GetRxFrequency() / 1e9 << std::endl;
		std::cout << " sample rate:" << usrp_controller_->GetRxSampleRate() / 1e6 << std::endl;
		std::cout << " gain:" << usrp_controller_->GetRxGain() << std::endl;

		usrp_initialized_.store(true);
		return true;
	} catch (const std::exception& e) {
		std::cerr << "Exception during USRP Initialization: " << e.what() << std::endl();
		return false
	}
}

void SignalGui::ShutdownUsrp() {
	if (usrp_controller_) {
		usrp_controller_->StopReceiving();
		usrp_controller_->Shutdown();
		usrp_controller_.reset();
	}
	usrp_initialized_.store(false);
}

// Real signals from USRP
void SignalGui::UsrpSamples() {
	if (!usrp_initialized.load() || !usrp_controller_) {
		for (int i = 0; i < 64; ++i) { signal_buffer_.Push(0.0f); }
		return ;
	}
	if (!usrp_controller_->IsReceiving()) {
		std::cout << "Starting sample rx..." << std::endl;
		auto callback = [this](const std::complex<float>* samples, size_t count) {
			this->ProcessUsrpSamples(samples, count);
		};
		if (!usrp_controller_->StartReceiving(callback, 8192)) {
			std::cerr << "Failed to start rx: " << usrp_controller_->GetLastError() << std::endl;
			return;
		}
	}

	samples_received_.store(usrp_controller_->GetTotalSamplesReceived());
	overflow_count_.store(usrp_controller_->GetOverflowCount());
	if (update_counter_ % 8 == 0) {
		UpdateUsrpFrequencyDomain();
	}
	if (update_counter_ % 16 == 0) {
		UpdateUsrpWaterfall();
	}
}

void SignalGui::ProcessUsrpSamples(const std::complex<float>* samples, size_t count) {
	for (size_t i = 0; i < count; ++i) {
		float real_sample = samples[i].real();
		signal_buffer_.Push(real_sample);
		current_time_ += 1.0f / sample_rate;
	}
}

void SignalGui::UpdateUsrpFrequencyDomain() {
}

void SignalGui::UpdateUsrpWaterfall() {
	float nyquist_freq = sample_rate_ / 2.0f;
	
	for (int f = 0; f < N_FREQ; ++f) {
		float freq = (float)f * nyquist_freq / N_FREQ;
		float intensity = -80.0f;
		
		if (usrp_frequency_ > 2.4e9 && usrp_frequency_ < 2.5e9) {
			if (freq > 1e6 && freq < 5e6) {
				intensity = -25.0f + 10.0f * std::sin(current_time_ * 0.5f + f * 0.1f);
			}
			if (freq > 8e6 && freq < 12e6) {
				intensity = -35.0f + 5.0f * std::sin(current_time_ * 0.3f + f * 0.05f);
			}
		}
		
		intensity += 5.0f * (float(rand()) / RAND_MAX - 0.5f);
		spectrogram_data[spectrogram_row_][f] = intensity;
	}
	spectrogram_row_ = (spectrogram_row_ + 1) % N_TIME_BINS;
}

// Simulated signal generator for testing GUI
void SignalGui::GenerateSamples() {
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

// Simulated signal updater
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
	if (mode_ == Mode::SIMULATION) { GenerateSamples(); }
	else						   { UsrpSamples();		}
	if (update_counter_ % 4 == 0) {
		UpdatePlotData();
	}

	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT), ImGuiCond_Always);


	ImGui::Begin("SigProc", nullptr,
			ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize);

	// Framerate
	ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);
	ImGui::SameLine();
	ImGui::Text("Mode: %s", (mode_ == Mode::SIMULATION) ? "SIM" : "USRP");

	if (mode_ == Mode::USRP) {
		ImGui::SameLine();
		if (usrp_initialized_.load()) {
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "CONNECTED");

			// Second line with USRP parameters
			ImGui::Text("USRP: %.3f GHz, %.1f MS/s, %.0f dB",
			           usrp_frequency_ / 1e9,
			           usrp_sample_rate_ / 1e6,
			           usrp_gain_);
			ImGui::SameLine();
			ImGui::Text("RX: %.1fM samples", samples_received_.load() / 1e6);
			if (overflow_count_.load() > 0) {
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(1, 0, 0, 1), "OVF: %zu", overflow_count_.load());
			}
		} else {
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "DISCONNECTED");
		}
	}

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
