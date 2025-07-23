#include "SignalGui.h"
#include "SDRFactory.h"
#include "FFTProcessor.h"
#include <glad/glad.h>
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
    
    // Initialize spectrogram analyzer with new PFFFT-based system
    int fft_size = 2048;  // Good balance of resolution and performance
    spectrogram_analyzer_ = std::make_unique<SpectrogramAnalyzer>(fft_size, sample_rate_);
    
    std::cout << "SignalGui initialized with " << sdr_device_->getDeviceType() 
              << " device, FFT size: " << fft_size << std::endl;
    
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
    
    // Feed samples to spectrogram analyzer
    if (spectrogram_analyzer_) {
        spectrogram_analyzer_->processSamples(samples, count);
        // Note: spectrum_ready_ will be updated in UpdateFrequencyDomain()
    }
    
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
    if (sdr_device_ && sdr_device_->isInitialized() && !sdr_device_->isReceiving()) {
        StartReceiving();
    }

	// Change update counter for faster updates per frame
    if (update_counter_ % 2 == 0) {
        UpdatePlotData();
    }
    if (update_counter_ % 4 == 0) {
        UpdateFrequencyDomain();
    }
    if (update_counter_ % 4 == 0) {
        UpdateWaterfall();
    }

    // Set window position and size
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT), ImGuiCond_Always);

    ImGui::Begin("SigProc", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

    // Render status bar
    RenderStatusBar();

    if (ImGui::BeginTabBar("MainTabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Multi-Plot View")) {
            RenderMultiPlotView();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("3D Spectrogram")) {
            Render3DSpectrogramView();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}


void SignalGui::RenderMultiPlotView() {
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
}

void SignalGui::Render3DSpectrogramView() {
    ImVec2 available_size = ImGui::GetContentRegionAvail();

    // Reserve space for controls at bottom
    float controls_height = 40.0f;  // Space for home button
    float display_height = available_size.y - controls_height;
    float display_width = available_size.x - 20.0f;  // Small padding

    ImGui::BeginChild("3DSpectrogram", available_size, true);

    // Lazy initialization with size validation
    if (!waterfall_3d_) {
        int render_width = std::max(512, static_cast<int>(display_width));
        int render_height = std::max(512, static_cast<int>(display_height));
        waterfall_3d_ = std::make_unique<Spectro3D>(render_width, render_height);
        
        if (!waterfall_3d_->isInitialized()) {
            ImGui::Text("Failed to initialize 3D waterfall renderer");
            ImGui::Text("Check that waterfall.vs and waterfall.fs exist in the working directory");
            ImGui::Text("Also verify OpenGL context is properly initialized");
            ImGui::EndChild();
            return;
        }
    }

    if (waterfall_3d_ && waterfall_3d_->isInitialized()) {
        // Render the 3D waterfall
        waterfall_3d_->render();

        // Display with mouse interaction
        unsigned int texture_id = waterfall_3d_->getTextureID();
        if (texture_id != 0) {
            ImVec2 image_pos = ImGui::GetCursorScreenPos();
            ImGui::Image((void*)(intptr_t)texture_id,
                        ImVec2(display_width, display_height));

            // Handle mouse interaction on the image
            if (ImGui::IsItemHovered()) {
                ImGuiIO& io = ImGui::GetIO();
                ImVec2 mouse_pos = ImGui::GetMousePos();
                double rel_x = mouse_pos.x - image_pos.x;
                double rel_y = mouse_pos.y - image_pos.y;

                // Normalize coordinates to [0,1] range for more predictable behavior
                double norm_x = rel_x / display_width;
                double norm_y = rel_y / display_height;
                
                // Clamp to valid range
                norm_x = std::max(0.0, std::min(1.0, norm_x));
                norm_y = std::max(0.0, std::min(1.0, norm_y));

                // Pass normalized coordinates scaled to internal size
                waterfall_3d_->handleMouseDrag(norm_x * 800.0, norm_y * 600.0,
                                              io.MouseDown[0], io.MouseDown[1]);

                if (io.MouseWheel != 0.0f) {
                    waterfall_3d_->handleMouseScroll(io.MouseWheel);
                }
            }
        } else {
            ImGui::Text("3D Renderer: No texture available");
            ImGui::Text("This might indicate an OpenGL context issue");
        }

        // Controls
        ImGui::Separator();
        if (ImGui::Button("Home")) {
            waterfall_3d_->resetView();
        }
        ImGui::SameLine();
        ImGui::Text("Mouse: Left=Rotate, Right=Pan, Wheel=Zoom");

    } else {
        ImGui::Text("3D Waterfall renderer not initialized");
        ImGui::Text("Check console for error messages");
    }

    ImGui::EndChild();
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
    if (!spectrogram_analyzer_) {
        // Use placeholder data if analyzer not ready
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

    // Try to get latest spectrum from analyzer
    std::vector<float> magnitudes(N_FREQ);
    spectrum_ready_ = spectrogram_analyzer_->getLatestSpectrum(magnitudes.data(), N_FREQ);

    // Try to get latest PSD from analyzer
    std::vector<float> psd_values(N_FREQ);
    bool psd_ready = spectrogram_analyzer_->getLatestPSD(psd_values.data(), N_FREQ, true); // dB scale

    if (spectrum_ready_) {
        // Generate frequency array with center frequency for SDR-style display
        std::vector<float> frequencies(N_FREQ);
        double center_freq = sdr_device_ ? sdr_device_->getFrequency() : 0.0;
        spectrogram_analyzer_->getFrequencyArray(frequencies.data(), N_FREQ, center_freq);

        // Update buffers with real data
        for (int i = 0; i < N_FREQ; ++i) {
            freq_buffer_.Push(frequencies[i]);
            magnitude_buffer_.Push(magnitudes[i]);

            // Use proper PSD if available, otherwise fallback to magnitude - 10dB
            if (psd_ready && i < static_cast<int>(psd_values.size())) {
                psd_buffer_.Push(psd_values[i]);
            } else {
                psd_buffer_.Push(magnitudes[i] - 10.0f); // Fallback
            }
        }
    } else {
        // Use placeholder data if no new spectrum available
        float nyquist_freq = sample_rate_ / 2.0f;

        for (int i = 0; i < N_FREQ; ++i) {
            float freq = (float)i * nyquist_freq / N_FREQ;
            freq_buffer_.Push(freq);

            float mag = -80.0f + 10.0f * (float(rand()) / RAND_MAX - 0.5f);
            magnitude_buffer_.Push(mag);
            psd_buffer_.Push(mag - 10.0f);
        }
    }
}

void SignalGui::UpdateWaterfall() {
	// Update 2D waterfall
    // Use the current magnitude data for waterfall
    if (spectrum_ready_ && magnitude_buffer_.Size() >= N_FREQ) {
        std::vector<float> current_magnitudes(N_FREQ);
        magnitude_buffer_.CopyLatest(current_magnitudes.data(), N_FREQ);

        // Copy to spectrogram row
        for (int f = 0; f < N_FREQ; ++f) {
            spectrogram_data[spectrogram_row_][f] = current_magnitudes[f];
        }
        spectrogram_row_ = (spectrogram_row_ + 1) % N_TIME_BINS;
    } else {
        // Fallback to placeholder if no data - but use better values
        for (int f = 0; f < N_FREQ; ++f) {
            float intensity = -70.0f + 5.0f * (float(rand()) / RAND_MAX - 0.5f);
            spectrogram_data[spectrogram_row_][f] = intensity;
        }
        spectrogram_row_ = (spectrogram_row_ + 1) % N_TIME_BINS;
    }
    
	// Update 3D waterfall with validation
	if (waterfall_3d_ && waterfall_3d_->isInitialized() && 
        spectrum_ready_ && magnitude_buffer_.Size() >= N_FREQ) {
        
        std::vector<float> current_magnitudes(N_FREQ);
        magnitude_buffer_.CopyLatest(current_magnitudes.data(), N_FREQ);
        
        // Validate data before passing to 3D renderer
        bool data_valid = true;
        for (int i = 0; i < N_FREQ; ++i) {
            if (!std::isfinite(current_magnitudes[i])) {
                data_valid = false;
                break;
            }
        }
        
        if (data_valid) {
            waterfall_3d_->updateWaterfallData(current_magnitudes.data(), N_FREQ);
        } else {
            static int error_count = 0;
            if (error_count++ < 5) {  // Limit error spam
                std::cerr << "Warning: Invalid magnitude data detected" << std::endl;
            }
        }
    }
}

void SignalGui::RenderTimeDomainPlot() {
    ImGui::Text("Time domain");
    if (ImPlot::BeginPlot("##TimePlot", ImVec2(-1, -1), ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText)) {
        // Auto-scale Y axis to signal data with fallback
        float min_amp = -1.0f, max_amp = 1.0f;
        if (signal_buffer_.Size() >= N_SAMPLES) {
            min_amp = signal_data[0];
            max_amp = signal_data[0];
            for (int i = 0; i < N_SAMPLES; ++i) {
                if (signal_data[i] < min_amp) min_amp = signal_data[i];
                if (signal_data[i] > max_amp) max_amp = signal_data[i];
            }
            // Add some padding
            float range = max_amp - min_amp;
            if (range > 0.001f) {  // Only scale if we have reasonable range
                float padding = range * 0.1f;
                min_amp -= padding;
                max_amp += padding;
            } else {
                // Fallback if range is too small
                min_amp = -0.1f;
                max_amp = 0.1f;
            }
        }
        
        // Calculate correct time range for the samples
        float time_duration = float(N_SAMPLES) / sample_rate_;  // Actual duration of samples
        
        ImPlot::SetupAxes("Time [s]", "Amplitude", 
                         ImPlotAxisFlags_NoMenus | ImPlotAxisFlags_Lock,
                         ImPlotAxisFlags_NoMenus | ImPlotAxisFlags_Lock);
        ImPlot::SetupAxesLimits(0.0, time_duration, min_amp, max_amp, ImGuiCond_Always);
        
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
    if (ImPlot::BeginPlot("##FreqPlot", ImVec2(-1, -1), ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText)) {
        // Auto-scale Y axis to data
        float min_mag = -80.0f, max_mag = -10.0f;
        if (magnitude_buffer_.Size() > 0) {
            min_mag = magnitude_data[0];
            max_mag = magnitude_data[0];
            for (int i = 0; i < N_FREQ; ++i) {
                if (magnitude_data[i] < min_mag) min_mag = magnitude_data[i];
                if (magnitude_data[i] > max_mag) max_mag = magnitude_data[i];
            }
            // Add some padding
            float padding = (max_mag - min_mag) * 0.1f;
            min_mag -= padding;
            max_mag += padding;
        }
        
        // Set frequency range and disable interactions
        double center_freq = sdr_device_ ? sdr_device_->getFrequency() : 0.0;
        float freq_min = center_freq;
        float freq_max = center_freq + sample_rate_ / 2.0f;
        
        ImPlot::SetupAxes("Frequency [Hz]", "Magnitude [dB]", 
                         ImPlotAxisFlags_NoMenus | ImPlotAxisFlags_Lock,
                         ImPlotAxisFlags_NoMenus | ImPlotAxisFlags_Lock);
        ImPlot::SetupAxesLimits(freq_min, freq_max, min_mag, max_mag, ImGuiCond_Always);
        
        ImPlot::PlotLine("Magnitude", freq_data, magnitude_data, N_FREQ);
        ImPlot::EndPlot();
    }
}

void SignalGui::RenderSpectrogramPlot() {
    ImGui::Text("Waterfall");
    if (ImPlot::BeginPlot("##SpectrogramPlot", ImVec2(-1, -1), ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText)) {
		// Heatmap min max values normalized
		// Make sure Incoming data is scaled properly
		float min_val = 0.0f;
		float max_val = 1.0f;
        
        // Calculate frequency range and lock axes
        double center_freq = sdr_device_ ? sdr_device_->getFrequency() : 0.0;
        float freq_min = center_freq;
        float freq_max = center_freq + sample_rate_ / 2.0f;
        
        ImPlot::SetupAxes("Frequency [Hz]", "Time", 
                         ImPlotAxisFlags_NoMenus | ImPlotAxisFlags_Lock, 
                         ImPlotAxisFlags_NoMenus | ImPlotAxisFlags_Lock | ImPlotAxisFlags_Invert);
        ImPlot::SetupAxesLimits(freq_min, freq_max, 0, N_TIME_BINS, ImGuiCond_Always);
        
        ImPlot::PlotHeatmap("##Waterfall",
                           (float*)spectrogram_data,
                           N_TIME_BINS, N_FREQ,
                           min_val, max_val,
                           nullptr,
                           ImPlotPoint(freq_min, 0),
                           ImPlotPoint(freq_max, N_TIME_BINS));
        ImPlot::EndPlot();
    }
}

void SignalGui::RenderPowerSpectralDensity() {
    ImGui::Text("Power spectral density");
    if (ImPlot::BeginPlot("##PSDPlot", ImVec2(-1, -1), ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText)) {
        // Auto-scale Y axis to PSD data
        float min_psd = -100.0f, max_psd = -20.0f;
        if (psd_buffer_.Size() > 0) {
            min_psd = psd_data[0];
            max_psd = psd_data[0];
            for (int i = 0; i < N_FREQ; ++i) {
                if (psd_data[i] < min_psd) min_psd = psd_data[i];
                if (psd_data[i] > max_psd) max_psd = psd_data[i];
            }
            // Add some padding
            float padding = (max_psd - min_psd) * 0.1f;
            min_psd -= padding;
            max_psd += padding;
        }
        
        // Set frequency range and disable interactions  
        double center_freq = sdr_device_ ? sdr_device_->getFrequency() : 0.0;
        float freq_min = center_freq;
        float freq_max = center_freq + sample_rate_ / 2.0f;
        
        ImPlot::SetupAxes("Frequency [Hz]", "PSD [dB/Hz]", 
                         ImPlotAxisFlags_NoMenus | ImPlotAxisFlags_Lock,
                         ImPlotAxisFlags_NoMenus | ImPlotAxisFlags_Lock);
        ImPlot::SetupAxesLimits(freq_min, freq_max, min_psd, max_psd, ImGuiCond_Always);
        
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
        
        // Recreate spectrogram analyzer with new sample rate
        int fft_size = 2048;
        spectrogram_analyzer_ = std::make_unique<SpectrogramAnalyzer>(fft_size, sample_rate_);
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
