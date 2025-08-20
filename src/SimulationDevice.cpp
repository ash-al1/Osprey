#include "SimulationDevice.h"
#include "SDRFactory.h"
#include <iostream>
#include <sstream>
#include <cmath>

static SDRDeviceRegistrar<SimulationDevice> sim_registrar("simulation");
static SDRDeviceRegistrar<SimulationDevice> sim_registrar_short("sim");

SimulationDevice::SimulationDevice() 
    : rng_(std::random_device{}()) {
    // Better tones for SDR display - spread across bandwidth
    addTone(-200e3, 0.5);  // 200 kHz below center
    addTone(150e3, 0.3);   // 150 kHz above center  
    addTone(50e3, 0.4);    // 50 kHz above center
    addTone(-350e3, 0.2);  // 350 kHz below center
}

SimulationDevice::~SimulationDevice() {
    if (isReceiving()) {
        stopReceiving();
    }
    shutdown();
}

bool SimulationDevice::initialize(const SDRConfig& config) {
    if (initialized_) {
        setError("Device already initialized");
        return false;
    }
    frequency_ = config.frequency;
    sample_rate_ = config.sample_rate;
    gain_ = config.gain;
    bandwidth_ = config.bandwidth > 0 ? config.bandwidth : config.sample_rate;
    antenna_ = config.antenna.empty() ? "SIM" : config.antenna;
    
    initialized_ = true;
    clearError();
    
    std::cout << "Simulation device initialized:" << std::endl;
    std::cout << "  Frequency: " << frequency_ / 1e6 << " MHz" << std::endl;
    std::cout << "  Sample rate: " << sample_rate_ / 1e6 << " MS/s" << std::endl;
    std::cout << "  Gain: " << gain_ << " dB" << std::endl;
    std::cout << "  Simulated tones at: -350, -200, +50, +150 kHz offsets" << std::endl;
    
    return true;
}

void SimulationDevice::shutdown() {
    if (isReceiving()) {
        stopReceiving();
    }
    initialized_ = false;
    clearError();
}

bool SimulationDevice::startReceiving(SampleCallback callback, size_t buffer_size) {
    if (!initialized_) {
        setError("Device not initialized");
        return false;
    }
    
    if (receiving_.load()) {
        setError("Already receiving");
        return false;
    }
    
    sample_callback_ = callback;
    buffer_size_ = buffer_size;
    stop_signal_.store(false);
    total_samples_.store(0);
    overflow_count_.store(0);
    start_time_ = std::chrono::steady_clock::now();
    
    try {
        generator_thread_ = std::make_unique<std::thread>(
            &SimulationDevice::generatorWorker, this);
        receiving_.store(true);
        std::cout << "Simulation device started generating samples" << std::endl;
        return true;
    } catch (const std::exception& e) {
        setError("Failed to start generator thread: " + std::string(e.what()));
        return false;
    }
}

void SimulationDevice::stopReceiving() {
    if (!receiving_.load()) return;
    
    std::cout << "Stopping simulation..." << std::endl;
    stop_signal_.store(true);
    
    if (generator_thread_ && generator_thread_->joinable()) {
        generator_thread_->join();
    }
    
    receiving_.store(false);
    generator_thread_.reset();
    
    auto duration = std::chrono::steady_clock::now() - start_time_;
    double seconds = std::chrono::duration<double>(duration).count();
    
    std::cout << "Simulation stopped:" << std::endl;
    std::cout << "  Total samples: " << total_samples_.load() << std::endl;
    std::cout << "  Duration: " << seconds << " s" << std::endl;
    std::cout << "  Effective rate: " << (total_samples_.load() / seconds) / 1e6 << " MS/s" << std::endl;
}

bool SimulationDevice::setFrequency(double freq_hz, size_t channel) {
    if (channel != 0) {
        setError("Invalid channel");
        return false;
    }
    frequency_ = freq_hz;
    return true;
}

bool SimulationDevice::setSampleRate(double rate_sps, size_t channel) {
    if (channel != 0) {
        setError("Invalid channel");
        return false;
    }
    if (rate_sps <= 0 || rate_sps > 100e6) {
        setError("Invalid sample rate");
        return false;
    }
    sample_rate_ = rate_sps;
    return true;
}

bool SimulationDevice::setGain(double gain_db, size_t channel) {
    if (channel != 0) {
        setError("Invalid channel");
        return false;
    }
    gain_ = gain_db;
    return true;
}

bool SimulationDevice::setBandwidth(double bandwidth_hz, size_t channel) {
    if (channel != 0) {
        setError("Invalid channel");
        return false;
    }
    bandwidth_ = bandwidth_hz;
    return true;
}

bool SimulationDevice::setAntenna(const std::string& antenna, size_t channel) {
    if (channel != 0) {
        setError("Invalid channel");
        return false;
    }
    antenna_ = antenna;
    return true;
}

std::string SimulationDevice::getDeviceInfo() const {
    std::stringstream ss;
    ss << "Simulation Device (no hardware required)\n";
    ss << "Serial: " << getSerialNumber() << "\n";
    ss << "Signal type: " << signal_type_ << "\n";
    ss << "Noise level: " << noise_level_ << "\n";
    ss << "Active tones: " << tones_.size() << "\n";
    return ss.str();
}

SDRCapabilities SimulationDevice::getCapabilities() const {
    SDRCapabilities caps;
    
    caps.min_frequency = 0.0;
    caps.max_frequency = 10e9;
    caps.min_sample_rate = 1e3;
    caps.max_sample_rate = 100e6;
    caps.min_gain = -100.0;
    caps.max_gain = 100.0;
    
    caps.has_adjustable_bandwidth = true;
    caps.has_bias_tee = false;
    caps.has_clock_source_selection = false;
    caps.num_channels = 1;
    
    caps.antennas = {"RX1"};
    
    return caps;
}

SDRStatus SimulationDevice::getStatus() const {
    SDRStatus status;
    
    status.initialized = initialized_;
    status.receiving = receiving_.load();
    status.current_frequency = frequency_;
    status.current_sample_rate = sample_rate_;
    status.current_gain = gain_;
    status.current_bandwidth = bandwidth_;
    status.samples_received = total_samples_.load();
    status.overflow_count = overflow_count_.load();
    status.has_overflow = status.overflow_count > 0;
    
    if (receiving_.load()) {
        auto duration = std::chrono::steady_clock::now() - start_time_;
        double seconds = std::chrono::duration<double>(duration).count();
        double expected_samples = sample_rate_ * seconds;
        status.reception_rate = (status.samples_received / expected_samples) * 100.0;
    }
    
    status.device_specific_status = "Signal: " + signal_type_;
    
    return status;
}

void SimulationDevice::addTone(double frequency, double amplitude) {
    Tone tone;
    tone.frequency = frequency;
    tone.amplitude = amplitude;
    tone.phase = 0.0;
    tones_.push_back(tone);
}

void SimulationDevice::clearTones() {
    tones_.clear();
}

void SimulationDevice::generatorWorker() {
    std::vector<std::complex<float>> buffer(buffer_size_);
    double time = 0.0;
    
    const double dt = 1.0 / sample_rate_;
    const auto batch_duration = std::chrono::duration<double>(buffer_size_ * dt);
    
    while (!stop_signal_.load()) {
        auto start = std::chrono::steady_clock::now();
        
        if (signal_type_ == "multitone") {
            generateMultitoneSamples(buffer.data(), buffer_size_, time);
        } else if (signal_type_ == "noise") {
            generateNoiseSamples(buffer.data(), buffer_size_);
        } else if (signal_type_ == "fm") {
            generateFMSamples(buffer.data(), buffer_size_, time);
        } else if (signal_type_ == "am") {
            generateAMSamples(buffer.data(), buffer_size_, time);
        } else {
            generateMultitoneSamples(buffer.data(), buffer_size_, time);
        }

        if (sample_callback_) {
            sample_callback_(buffer.data(), buffer_size_);
            total_samples_.fetch_add(buffer_size_);
        }
        
        auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed < batch_duration) {
            std::this_thread::sleep_for(batch_duration - elapsed);
        } else {
            overflow_count_.fetch_add(1);
        }
    }
}

void SimulationDevice::generateMultitoneSamples(std::complex<float>* buffer, 
                                                size_t count, double& time) {
    const double dt = 1.0 / sample_rate_;
    const double gain_linear = std::pow(10.0, gain_ / 20.0);
    
    for (size_t i = 0; i < count; ++i) {
        float real = 0.0f;
        float imag = 0.0f;
        
        // Generate fixed tones
        for (auto& tone : tones_) {
            double phase_increment = 2.0 * M_PI * tone.frequency * dt;
            real += tone.amplitude * std::cos(tone.phase);
            imag += tone.amplitude * std::sin(tone.phase);
            tone.phase += phase_increment;
            
            if (tone.phase > 2.0 * M_PI) {
                tone.phase -= 2.0 * M_PI;
            }
        }
        
        // Add a sweeping tone that moves across the spectrum
		double sweep_freq = 300e3 * std::sin(time * 0.05);
        double sweep_phase = 2.0 * M_PI * sweep_freq * time;
        real += (0.3f / 200.0) * std::cos(sweep_phase);
        imag += (0.3f / 200.0) * std::sin(sweep_phase);
        
        // Add some bandwidth around center frequency
		double wide_signal_freq = 100e3 * (noise_dist_(rng_) * 0.1);
        double wide_phase = 2.0 * M_PI * wide_signal_freq * time;
        real += (0.1f / 200.0) * std::cos(wide_phase);
        imag += (0.1f / 200.0) * std::sin(wide_phase);
        
        // Add noise
        real += noise_level_ * noise_dist_(rng_);
        imag += noise_level_ * noise_dist_(rng_);
        
        buffer[i] = std::complex<float>(real * gain_linear, imag * gain_linear);
        time += dt;
    }
}

void SimulationDevice::generateNoiseSamples(std::complex<float>* buffer, size_t count) {
    const double gain_linear = std::pow(10.0, gain_ / 20.0);
    
    for (size_t i = 0; i < count; ++i) {
        float real = noise_dist_(rng_) * gain_linear;
        float imag = noise_dist_(rng_) * gain_linear;
        buffer[i] = std::complex<float>(real, imag);
    }
}

void SimulationDevice::generateFMSamples(std::complex<float>* buffer, 
                                         size_t count, double& time) {
    const double dt = 1.0 / sample_rate_;
    const double gain_linear = std::pow(10.0, gain_ / 20.0);
    const double carrier_freq = 1e5;  // 100 kHz offset
    const double mod_freq = 1e3;      // 1 kHz modulation
    const double mod_index = 50e3;    // 50 kHz deviation
    
    static double carrier_phase = 0.0;
    
    for (size_t i = 0; i < count; ++i) {
        double mod_signal = mod_index * std::sin(2.0 * M_PI * mod_freq * time);
        double instantaneous_freq = carrier_freq + mod_signal;

        carrier_phase += 2.0 * M_PI * instantaneous_freq * dt;
        
        float real = gain_linear * std::cos(carrier_phase);
        float imag = gain_linear * std::sin(carrier_phase);
        
        real += noise_level_ * noise_dist_(rng_);
        imag += noise_level_ * noise_dist_(rng_);
        
        buffer[i] = std::complex<float>(real, imag);
        time += dt;
        if (carrier_phase > 2.0 * M_PI) {
            carrier_phase -= 2.0 * M_PI;
        }
    }
}

void SimulationDevice::generateAMSamples(std::complex<float>* buffer, 
                                         size_t count, double& time) {
    const double dt = 1.0 / sample_rate_;
    const double gain_linear = std::pow(10.0, gain_ / 20.0);
    const double carrier_freq = 200e3;  // 200 kHz offset
    const double mod_freq = 5e3;        // 5 kHz modulation
    const double mod_depth = 0.8;
    
    for (size_t i = 0; i < count; ++i) {
        double carrier = std::cos(2.0 * M_PI * carrier_freq * time);
        double modulation = 1.0 + mod_depth * std::sin(2.0 * M_PI * mod_freq * time);
        
        float real = gain_linear * modulation * carrier;
        float imag = 0.0f;
        
        real += noise_level_ * noise_dist_(rng_);
        imag += noise_level_ * noise_dist_(rng_);
        buffer[i] = std::complex<float>(real, imag);
        time += dt;
    }
}
