#pragma once

#include <complex>
#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <iostream>
#include <vector>

struct SDRConfig;
struct SDRCapabilities;
struct SDRStatus;

/**
 * Abstract class or any SDR device
 * Unified interface regardless of hardware
 */
class SDRDevice {
public:
    using SampleCallback = std::function<void(const std::complex<float>*, size_t)>;
    
    // Constructor/Destructor
    SDRDevice() = default;
    virtual ~SDRDevice() = default;
    
    // Core
    virtual bool initialize(const SDRConfig& config) = 0;
    virtual void shutdown() = 0;
    virtual bool isInitialized() const = 0;
    
    // RX
    virtual bool startReceiving(SampleCallback callback, size_t buffer_size = 4096) = 0;
    virtual void stopReceiving() = 0;
    virtual bool isReceiving() const = 0;
    
    // Params
    virtual bool setFrequency(double freq_hz, size_t channel = 0) = 0;
    virtual bool setSampleRate(double rate_sps, size_t channel = 0) = 0;
    virtual bool setGain(double gain_db, size_t channel = 0) = 0;
    virtual bool setBandwidth(double bandwidth_hz, size_t channel = 0) = 0;
    virtual bool setAntenna(const std::string& antenna, size_t channel = 0) = 0;
    
    // Getters
    virtual double getFrequency(size_t channel = 0) const = 0;
    virtual double getSampleRate(size_t channel = 0) const = 0;
    virtual double getGain(size_t channel = 0) const = 0;
    virtual double getBandwidth(size_t channel = 0) const = 0;
    virtual std::string getAntenna(size_t channel = 0) const = 0;
    
    // Device info
    virtual std::string getDeviceType() const = 0;
    virtual std::string getSerialNumber() const = 0;
    virtual std::string getDeviceInfo() const = 0;
    virtual SDRCapabilities getCapabilities() const = 0;
    
    // Statistics
    virtual SDRStatus getStatus() const = 0;
    virtual size_t getTotalSamplesReceived() const = 0;
    virtual size_t getOverflowCount() const = 0;
    
    // Error
    virtual std::string getLastError() const = 0;
    virtual void clearError() = 0;
    
protected:
    mutable std::string last_error_;
    void setError(const std::string& error) const {
        last_error_ = error;
        if (!error.empty()) {
			std::cerr << "ERROR: " << error << std::endl;
        }
    }
};

/**
 * SDR device configuration
 */
struct SDRConfig {
    std::string device_type;
    std::string serial_number;
    
    // Params
    double frequency = 100e6;
    double sample_rate = 1e6;
    double gain = 20.0;
    double bandwidth = 0.0;
    std::string antenna = "";
    
    // Internals
    std::string clock_source = "internal";
    std::string time_source = "internal";
    size_t channel = 0;		// Useful for multi channel devices
    
    // Performance tuning
    size_t buffer_size = 4096;
    size_t num_buffers = 64;
};

/**
 * Device hardware capabilities
 */
struct SDRCapabilities {
    // Frequency
    double min_frequency = 0.0;
    double max_frequency = 0.0;
    
    // SPS
    double min_sample_rate = 0.0;
    double max_sample_rate = 0.0;
    
    // Gain
    double min_gain = 0.0;
    double max_gain = 0.0;
    
    bool has_adjustable_bandwidth = false;
    bool has_bias_tee = false;
    bool has_clock_source_selection = false;
    size_t num_channels = 1;
    
    // Antennas
    std::vector<std::string> antennas;
};

/**
 * Runtime
 */
struct SDRStatus {
    bool initialized = false;
    bool receiving = false;
    bool has_overflow = false;
    
    // Current
    double current_frequency = 0.0;
    double current_sample_rate = 0.0;
    double current_gain = 0.0;
    double current_bandwidth = 0.0;
    
    // Performance
    size_t samples_received = 0;
    size_t overflow_count = 0;
    double reception_rate = 0.0;
    
    // status
    std::string device_specific_status;
};
