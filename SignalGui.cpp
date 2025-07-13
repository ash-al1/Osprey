#include "SignalGui.h"
#include "SDRFactory.h"
#include "FFTProcessor.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <iomanip>

SignalGui::SignalGui()
    : time_buffer_(N_SAMPLES)
    , signal_buffer_(N_SAMPLES)
    , freq_buffer_(N_FREQ)
    , magnitude_buffer_(N_FREQ)
    , psd_buffer_(N_FREQ)
    , current_time_(0.0f)
    , sample_rate_(1000.0f)
    , spectrogram_row_(0)
    , update_counter_(0)
    , samples_received_(0)
    , overflow_count_(0) {
    
    // Initialize spectrogram data
    for (int t = 0; t < N_TIME_BINS; ++t) {
        for (int f = 0; f < N_FREQ; ++f) {
            spectrogram_data[t][f] = -80.0f;
        }
    }
}

SignalGui::~SignalGui() {
    if (sdr_device_ && sdr_device_->isReceiving()) {
        sdr_device_->stopReceiving();
    }
}

bool SignalGui::Initialize(const SDRConfig& config) {
    device_config_ = config;
    
    // Create and initialize the SDR device
    sdr_device_ = SDRFactory::createAndInitialize(config);
    
    if (!sdr_device_) {
        std::cerr << "Failed to create " << config.device_type << " device" << std::endl;
        return false;
    }
    
    // Update sample rate from device
    sample_rate_ = static_cast<float>(sdr_device_->getSampleRate());
    
    // Initialize spectrum analyzer
    SpectrumAnalyzer::Config spectrum_config;
    spectrum_config.fft_size = N_FREQ * 2;  // Use larger FFT for better resolution
    spectrum_config.window_type = FFTProcessor::WindowType::HAMMING;
    spectrum_config.averaging_count = 4;
    spectrum_config.overlap_ratio = 0.5f;
    spectrum_config.remove_dc = true;
    
    spectrum_analyzer_ = std::make_unique<SpectrumAnalyzer>(spectrum_config, sample_rate_);
    
    std::cout << "SignalGui initialized with " << sdr_device_->getDeviceType() 
              << " device" << std::endl;
    
    return true;
}

bool SignalGui::StartReceiving() {
    if (!sdr_device_ || !sdr_device_->isInitialized()) {
        std::cerr << "Device not initialized" << std::endl;
        return false;
    }
    
    // Reset counters
    samples_received_.store(0);
    overflow_count_.store(0);
    
    // Start receiving with our callback
    auto callback = [this](const std::complex<float>* samples, size_t count) {
        this->ProcessSamples(samples, count);
    };
    
    return sdr_device_->startReceiving(callback, device_config_.buffer_size);
}

void SignalGui::StopReceiving() {
    if (sdr_device_) {
        sdr_device_->stopReceiving();
    }
}

bool SignalGui::IsReceiving() const {
    return sdr_device_ && sdr_device_->isReceiving();
}

void SignalGui::ProcessSamples(const std::complex<float>* samples, size_t count) {
    // Extract real part for time domain display
    for (size_t i = 0; i < count; ++i) {
        float real_sample = samples[i].real();
        signal_buffer_.Push(real_sample);
        current_time_ += 1.0f / sample_rate_;
    }
    
	/**
    // Feed samples to spectrum analyzer
    if (spectrum_analyzer_) {
        spectrum_analyzer_->addSamples(samples, count);
        spectrum_ready_ = spectrum_analyzer_->isSpectrumReady();
    }*/
    
    samples_received_.fetch_add(count);
    
    // Check device statistics for overflows
    if (sdr_device_) {
        size_t device_overflows = sdr_device_->getOverflowCount();
        if (device_overflows > overflow_count_.load()) {
            overflow_count_.store(device_overflows);
        }
    }
}

void SignalGui::Update() {
    update_counter_++;
    
    // Start receiving if not already (for devices that need continuous operation)
    if (sdr_device_ && sdr_device_->isInitialized() && !sdr_device_->isReceiving()) {
        StartReceiving();
    }
    
    // Update plot data periodically
    if (update_counter_ % 4 == 0) {
        UpdatePlotData();
    }
    
    if (update_counter_ % 8 == 0) {
        UpdateFrequencyDomain();
    }
    
    if (update_counter_ % 16 == 0) {
        UpdateWaterfall();
    }
    
    // Set window position and size
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT), ImGuiCond_Always);
    
    ImGui::Begin("SigProc", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
    
    // Render status bar
    RenderStatusBar();
    
    // Calculate dimensions for each plot
    float quarter_width  = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
    float quarter_height = (ImGui::GetContentRegionAvail().y - ImGui::GetStyle().ItemSpacing.y) * 0.5f;
    
    // Top Left - Time Domain
    ImGui::BeginChild("TimeDomain", ImVec2(quarter_width, quarter_height), true);
    RenderTimeDomainPlot();
    ImGui::EndChild();
    ImGui::SameLine();
    
    // Top Right - Frequency Domain
    ImGui::BeginChild("Frequency", ImVec2(quarter_width, quarter_height), true);
    RenderFrequencyPlot();
    ImGui::EndChild();
    
    // Bottom Left - Spectrogram
    ImGui::BeginChild("Spectrogram", ImVec2(quarter_width, quarter_height), true);
    RenderSpectrogramPlot();
    ImGui::EndChild();
    ImGui::SameLine();
    
    // Bottom Right - PSD
    ImGui::BeginChild("PSD", ImVec2(quarter_width, quarter_height), true);
    RenderPowerSpectralDensity();
    ImGui::EndChild();
    
    ImGui::End();
}

void SignalGui::RenderStatusBar() {
    // First line - basic info
    ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);
    ImGui::SameLine();
    
    if (sdr_device_) {
        ImGui::Text("Device: %s", sdr_device_->getDeviceType().c_str());
        ImGui::SameLine();
        
        if (sdr_device_->isInitialized()) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "CONNECTED");
            
            // Second line - device parameters
            SDRStatus status = sdr_device_->getStatus();
            ImGui::Text("%s: %.3f GHz, %.1f MS/s, %.0f dB",
                       sdr_device_->getDeviceType().c_str(),
                       status.current_frequency / 1e9,
                       status.current_sample_rate / 1e6,
                       status.current_gain);
            
            ImGui::SameLine();
            ImGui::Text("RX: %.1fM samples", samples_received_.load() / 1e6);
            
            if (overflow_count_.load() > 0) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "OVF: %zu", overflow_count_.load());
            }
            
            // Show reception rate if receiving
            if (status.receiving && status.reception_rate > 0) {
                ImGui::SameLine();
                ImGui::Text("Rate: %.1f%%", status.reception_rate);
            }
        } else {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "DISCONNECTED");
        }
    } else {
        ImGui::Text("No device");
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

void SignalGui::UpdateFrequencyDomain() {
    if (!spectrum_analyzer_ || !spectrum_ready_) {
        // Use placeholder data if spectrum not ready
        float nyquist_freq = sample_rate_ / 2.0f;
        
        for (int i = 0; i < N_FREQ; ++i) {
            float freq = (float)i * nyquist_freq / N_FREQ;
            freq_buffer_.Push(freq);
            
            float mag = -80.0f + 10.0f * (float(rand()) / RAND_MAX - 0.5f);
            magnitude_buffer_.Push(mag);
            psd_buffer_.Push(mag - 10.0f);
        }
        return;
    }
    
    // Get real FFT results
    std::vector<float> frequencies(N_FREQ);
    std::vector<float> magnitudes(N_FREQ);
    std::vector<float> psd_values(N_FREQ);
    
    spectrum_analyzer_->getFrequencies(frequencies.data(), N_FREQ);
    spectrum_analyzer_->getMagnitudeSpectrum(magnitudes.data(), N_FREQ);
    spectrum_analyzer_->getPowerSpectralDensity(psd_values.data(), N_FREQ);
    
    // Update buffers
    for (int i = 0; i < N_FREQ; ++i) {
        freq_buffer_.Push(frequencies[i]);
        magnitude_buffer_.Push(magnitudes[i]);
        psd_buffer_.Push(psd_values[i]);
    }
    
    spectrum_ready_ = false;  // Reset for next cycle
}

void SignalGui::UpdateWaterfall() {
    // Use the current magnitude data for waterfall
    if (magnitude_buffer_.Size() >= N_FREQ) {
        std::vector<float> current_magnitudes(N_FREQ);
        magnitude_buffer_.CopyLatest(current_magnitudes.data(), N_FREQ);
        
        // Copy to spectrogram row
        for (int f = 0; f < N_FREQ; ++f) {
            spectrogram_data[spectrogram_row_][f] = current_magnitudes[f];
        }
        spectrogram_row_ = (spectrogram_row_ + 1) % N_TIME_BINS;
    } else {
        // Fallback to placeholder if no data
        float nyquist_freq = sample_rate_ / 2.0f;
        
        for (int f = 0; f < N_FREQ; ++f) {
            float freq = (float)f * nyquist_freq / N_FREQ;
            float intensity = -80.0f;
            
            if (GetDeviceType() == "simulation" || GetDeviceType() == "sim") {
                float varying_freq = 50.0f + 15.0f * std::sin(current_time_ * 0.3f);
                if (std::abs(freq - varying_freq) < 8.0f) {
                    intensity = -20.0f;
                }
                if (std::abs(freq - 150.0f) < 5.0f) {
                    intensity = -30.0f;
                }
            }
            
            intensity += 5.0f * (float(rand()) / RAND_MAX - 0.5f);
            spectrogram_data[spectrogram_row_][f] = intensity;
        }
        spectrogram_row_ = (spectrogram_row_ + 1) % N_TIME_BINS;
    }
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
    if (ImPlot::BeginPlot("##FreqPlot", ImVec2(-1, -1))) {
        ImPlot::SetupAxes("Frequency [Hz]", "Magnitude [dB]");
        
        // Set appropriate frequency range based on sample rate
        float max_freq = std::min(sample_rate_ / 2.0f, 250.0f);
        ImPlot::SetupAxesLimits(0, max_freq, -80, 0);
        
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

void SignalGui::RenderPowerSpectralDensity() {
    ImGui::Text("Power spectral density");
    if (ImPlot::BeginPlot("##PSDPlot", ImVec2(-1, -1))) {
        ImPlot::SetupAxes("Frequency [Hz]", "PSD [dB/Hz]");
        
        float max_freq = std::min(sample_rate_ / 2.0f, 250.0f);
        ImPlot::SetupAxesLimits(0, max_freq, -90, -20);
        
        ImPlot::PlotLine("PSD", freq_data, psd_data, N_FREQ);
        ImPlot::EndPlot();
    }
}

// Device control methods
bool SignalGui::SetFrequency(double freq_hz) {
    if (!sdr_device_) return false;
    
    bool success = sdr_device_->setFrequency(freq_hz);
    if (success) {
        device_config_.frequency = freq_hz;
    }
    return success;
}

bool SignalGui::SetSampleRate(double rate_sps) {
    if (!sdr_device_) return false;
    
    bool success = sdr_device_->setSampleRate(rate_sps);
    if (success) {
        device_config_.sample_rate = rate_sps;
        sample_rate_ = static_cast<float>(rate_sps);
        
        // Recreate spectrum analyzer with new sample rate
        SpectrumAnalyzer::Config spectrum_config;
        spectrum_config.fft_size = N_FREQ * 2;
        spectrum_config.window_type = FFTProcessor::WindowType::HAMMING;
        spectrum_config.averaging_count = 4;
        spectrum_config.overlap_ratio = 0.5f;
        spectrum_config.remove_dc = true;
        
        spectrum_analyzer_ = std::make_unique<SpectrumAnalyzer>(spectrum_config, sample_rate_);
    }
    return success;
}

bool SignalGui::SetGain(double gain_db) {
    if (!sdr_device_) return false;
    
    bool success = sdr_device_->setGain(gain_db);
    if (success) {
        device_config_.gain = gain_db;
    }
    return success;
}

bool SignalGui::SetBandwidth(double bandwidth_hz) {
    if (!sdr_device_) return false;
    
    bool success = sdr_device_->setBandwidth(bandwidth_hz);
    if (success) {
        device_config_.bandwidth = bandwidth_hz;
    }
    return success;
}

// Info methods
std::string SignalGui::GetDeviceType() const {
    return sdr_device_ ? sdr_device_->getDeviceType() : "none";
}

std::string SignalGui::GetDeviceInfo() const {
    return sdr_device_ ? sdr_device_->getDeviceInfo() : "No device";
}

SDRStatus SignalGui::GetDeviceStatus() const {
    if (sdr_device_) {
        return sdr_device_->getStatus();
    }
    return SDRStatus();
}

SDRCapabilities SignalGui::GetDeviceCapabilities() const {
    if (sdr_device_) {
        return sdr_device_->getCapabilities();
    }
    return SDRCapabilities();
}
