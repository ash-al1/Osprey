#pragma once

#include "SDRDevice.h"
#include <thread>
#include <atomic>
#include <random>
#include <chrono>

/**
 * Simulated SDR device
 * Generates synthetic signals
 */
class SimulationDevice : public SDRDevice {
public:
    SimulationDevice();
    ~SimulationDevice() override;
    
    // Core
    bool initialize(const SDRConfig& config) override;
    void shutdown() override;
    bool isInitialized() const override { return initialized_; }
    
    // Rx
    bool startReceiving(SampleCallback callback, size_t buffer_size = 4096) override;
    void stopReceiving() override;
    bool isReceiving() const override { return receiving_.load(); }
    
    // Setters
    bool setFrequency(double freq_hz, size_t channel = 0) override;
    bool setSampleRate(double rate_sps, size_t channel = 0) override;
    bool setGain(double gain_db, size_t channel = 0) override;
    bool setBandwidth(double bandwidth_hz, size_t channel = 0) override;
    bool setAntenna(const std::string& antenna, size_t channel = 0) override;
    
    // Getters
    double getFrequency(size_t channel = 0) const override { return frequency_; }
    double getSampleRate(size_t channel = 0) const override { return sample_rate_; }
    double getGain(size_t channel = 0) const override { return gain_; }
    double getBandwidth(size_t channel = 0) const override { return bandwidth_; }
    std::string getAntenna(size_t channel = 0) const override { return antenna_; }
    
    // Device info
    std::string getDeviceType() const override { return "simulation"; }
    std::string getSerialNumber() const override { return "SIM-001"; }
    std::string getDeviceInfo() const override;
    SDRCapabilities getCapabilities() const override;
    
    // Statistics
    SDRStatus getStatus() const override;
    size_t getTotalSamplesReceived() const override { return total_samples_.load(); }
    size_t getOverflowCount() const override { return overflow_count_.load(); }
    
    // Error
    std::string getLastError() const override { return last_error_; }
    void clearError() override { last_error_.clear(); }
    
    void setSignalType(const std::string& type) { signal_type_ = type; }
    void setNoiseLevel(double level) { noise_level_ = level; }
    void addTone(double frequency, double amplitude);
    void clearTones();
    
private:
    bool initialized_ = false;
    double frequency_ = 100e6;
    double sample_rate_ = 1e6;
    double gain_ = 20.0;
    double bandwidth_ = 0.0;
    std::string antenna_ = "SIM";
    
    std::string signal_type_ = "multitone";
    double noise_level_ = 0.1;
    struct Tone {
        double frequency;
        double amplitude;
        double phase;
    };
    std::vector<Tone> tones_;
    
    // Threading
    std::atomic<bool> receiving_{false};
    std::atomic<bool> stop_signal_{false};
    std::unique_ptr<std::thread> generator_thread_;
    SampleCallback sample_callback_;
    size_t buffer_size_ = 4096;
    
    // Statistics
    std::atomic<size_t> total_samples_{0};
    std::atomic<size_t> overflow_count_{0};
    std::chrono::steady_clock::time_point start_time_;
    
    // Random number generation
    std::mt19937 rng_;
    std::normal_distribution<float> noise_dist_{0.0f, 1.0f};
    
    // Thread
    void generatorWorker();
    
    // Signal generation functions
    void generateMultitoneSamples(std::complex<float>* buffer, size_t count, double& time);
    void generateNoiseSamples(std::complex<float>* buffer, size_t count);
    void generateFMSamples(std::complex<float>* buffer, size_t count, double& time);
    void generateAMSamples(std::complex<float>* buffer, size_t count, double& time);
};
