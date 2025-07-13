#pragma once

#include "SDRDevice.h"
#include "UsrpController.h"
#include <memory>

/**
 * USRP implementation of the SDRDevice interface
 * Wraps the existing UsrpController functionality
 */
class USRPDevice : public SDRDevice {
public:
    USRPDevice();
    ~USRPDevice() override;
    
    // Core
    bool initialize(const SDRConfig& config) override;
    void shutdown() override;
    bool isInitialized() const override;
    
    // Rx
    bool startReceiving(SampleCallback callback, size_t buffer_size = 4096) override;
    void stopReceiving() override;
    bool isReceiving() const override;
    
    // Setters
    bool setFrequency(double freq_hz, size_t channel = 0) override;
    bool setSampleRate(double rate_sps, size_t channel = 0) override;
    bool setGain(double gain_db, size_t channel = 0) override;
    bool setBandwidth(double bandwidth_hz, size_t channel = 0) override;
    bool setAntenna(const std::string& antenna, size_t channel = 0) override;
    
    // Getters
    double getFrequency(size_t channel = 0) const override;
    double getSampleRate(size_t channel = 0) const override;
    double getGain(size_t channel = 0) const override;
    double getBandwidth(size_t channel = 0) const override;
    std::string getAntenna(size_t channel = 0) const override;
    
    // Device info
    std::string getDeviceType() const override { return "usrp"; }
    std::string getSerialNumber() const override;
    std::string getDeviceInfo() const override;
    SDRCapabilities getCapabilities() const override;
    
    // Statistics
    SDRStatus getStatus() const override;
    size_t getTotalSamplesReceived() const override;
    size_t getOverflowCount() const override;
    
    // Error
    std::string getLastError() const override;
    void clearError() override;
    
    double getMasterClockRate() const;
    bool setClockSource(const std::string& source);
    bool setTimeSource(const std::string& source);
    
    static std::vector<SDRConfig> detectUSRPDevices();
    
private:
    std::unique_ptr<UsrpController> controller_;
    SDRConfig current_config_;
    
    void syncError() const;
};
