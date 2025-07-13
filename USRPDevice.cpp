#include "USRPDevice.h"
#include "SDRFactory.h"
#include <iostream>

static SDRDeviceRegistrar<USRPDevice> usrp_registrar("usrp");

USRPDevice::USRPDevice() 
    : controller_(std::make_unique<UsrpController>()) {
}

USRPDevice::~USRPDevice() {
    if (isReceiving()) {
        stopReceiving();
    }
    shutdown();
}

bool USRPDevice::initialize(const SDRConfig& config) {
    if (!controller_) {
        setError("USRP controller not allocated");
        return false;
    }
    current_config_ = config;
    bool success = controller_->Initialize(config.serial_number);
    if (!success) {
        syncError();
        return false;
    }
    
    if (!config.clock_source.empty()) {
        controller_->SetClockSource(config.clock_source);
    }
    if (!config.time_source.empty()) {
        controller_->SetTimeSource(config.time_source);
    }
    clearError();
    return true;
}

void USRPDevice::shutdown() {
    if (controller_) {
        controller_->Shutdown();
    }
    current_config_ = SDRConfig();
}

bool USRPDevice::isInitialized() const {
    return controller_ && controller_->IsInitialized();
}

bool USRPDevice::startReceiving(SampleCallback callback, size_t buffer_size) {
    if (!controller_) {
        setError("Device not initialized");
        return false;
    }
    bool success = controller_->StartReceiving(callback, buffer_size);
    if (!success) {
        syncError();
    }
    return success;
}

void USRPDevice::stopReceiving() {
    if (controller_) {
        controller_->StopReceiving();
    }
}

bool USRPDevice::isReceiving() const {
    return controller_ && controller_->IsReceiving();
}

bool USRPDevice::setFrequency(double freq_hz, size_t channel) {
    if (!controller_) {
        setError("Device not initialized");
        return false;
    }
    bool success = controller_->SetRxFrequency(freq_hz, channel);
    if (!success) {
        syncError();
    } else {
        current_config_.frequency = freq_hz;
    }
    return success;
}

bool USRPDevice::setSampleRate(double rate_sps, size_t channel) {
    if (!controller_) {
        setError("Device not initialized");
        return false;
    }
    bool success = controller_->SetRxSampleRate(rate_sps, channel);
    if (!success) {
        syncError();
    } else {
        current_config_.sample_rate = rate_sps;
    }
    return success;
}

bool USRPDevice::setGain(double gain_db, size_t channel) {
    if (!controller_) {
        setError("Device not initialized");
        return false;
    }
    bool success = controller_->SetRxGain(gain_db, channel);
    if (!success) {
        syncError();
    } else {
        current_config_.gain = gain_db;
    }
    return success;
}

bool USRPDevice::setBandwidth(double bandwidth_hz, size_t channel) {
    if (!controller_) {
        setError("Device not initialized");
        return false;
    }
    bool success = controller_->SetRxBandwidth(bandwidth_hz, channel);
    if (!success) {
        syncError();
    } else {
        current_config_.bandwidth = bandwidth_hz;
    }
    return success;
}

bool USRPDevice::setAntenna(const std::string& antenna, size_t channel) {
    if (!controller_) {
        setError("Device not initialized");
        return false;
    }
    bool success = controller_->SetRxAntenna(antenna, channel);
    if (!success) {
        syncError();
    } else {
        current_config_.antenna = antenna;
    }
    return success;
}

double USRPDevice::getFrequency(size_t channel) const {
    return controller_ ? controller_->GetRxFrequency(channel) : 0.0;
}

double USRPDevice::getSampleRate(size_t channel) const {
    return controller_ ? controller_->GetRxSampleRate(channel) : 0.0;
}

double USRPDevice::getGain(size_t channel) const {
    return controller_ ? controller_->GetRxGain(channel) : 0.0;
}

double USRPDevice::getBandwidth(size_t channel) const {
    return controller_ ? controller_->GetRxBandwidth(channel) : 0.0;
}

std::string USRPDevice::getAntenna(size_t channel) const {
    return controller_ ? controller_->GetRxAntenna(channel) : "";
}

std::string USRPDevice::getSerialNumber() const {
    return controller_ ? controller_->GetSerialNumber() : "";
}

std::string USRPDevice::getDeviceInfo() const {
    return controller_ ? controller_->GetDeviceInfo() : "";
}

SDRCapabilities USRPDevice::getCapabilities() const {
    SDRCapabilities caps;
    caps.min_frequency = 70e6;
    caps.max_frequency = 6e9;
    caps.min_sample_rate = 200e3;
    caps.max_sample_rate = 61.44e6;
    caps.min_gain = 0.0;
    caps.max_gain = 76.0;
    caps.has_adjustable_bandwidth = true;
    caps.has_bias_tee = false;
    caps.has_clock_source_selection = true;
    caps.num_channels = 1;		// This really has 2, but fix to 1 for use case
    caps.antennas = {"TX/RX", "RX2"};
    return caps;
}

SDRStatus USRPDevice::getStatus() const {
    SDRStatus status;
    if (controller_) {
        status.initialized = controller_->IsInitialized();
        status.receiving = controller_->IsReceiving();
        status.current_frequency = controller_->GetRxFrequency();
        status.current_sample_rate = controller_->GetRxSampleRate();
        status.current_gain = controller_->GetRxGain();
        status.current_bandwidth = controller_->GetRxBandwidth();
        status.samples_received = controller_->GetTotalSamplesReceived();
        status.overflow_count = controller_->GetOverflowCount();
        status.has_overflow = status.overflow_count > 0;
		// This is fake stat
        if (status.receiving && status.current_sample_rate > 0) {
            double expected_samples = status.current_sample_rate * 5.0;
            status.reception_rate = (status.samples_received / expected_samples) * 100.0;
        }
    }
    return status;
}

size_t USRPDevice::getTotalSamplesReceived() const {
    return controller_ ? controller_->GetTotalSamplesReceived() : 0;
}

size_t USRPDevice::getOverflowCount() const {
    return controller_ ? controller_->GetOverflowCount() : 0;
}

std::string USRPDevice::getLastError() const {
    return last_error_.empty() && controller_ ? controller_->GetLastError() : last_error_;
}

void USRPDevice::clearError() {
    last_error_.clear();
}

double USRPDevice::getMasterClockRate() const {
    return controller_ ? controller_->GetMasterClockRate() : 0.0;
}

bool USRPDevice::setClockSource(const std::string& source) {
    if (!controller_) {
        setError("Device not initialized");
        return false;
    }
    bool success = controller_->SetClockSource(source);
    if (!success) {
        syncError();
    }
    return success;
}

bool USRPDevice::setTimeSource(const std::string& source) {
    if (!controller_) {
        setError("Device not initialized");
        return false;
    }
    bool success = controller_->SetTimeSource(source);
    if (!success) {
        syncError();
    }
    return success;
}

void USRPDevice::syncError() const {
    if (controller_) {
        std::string err = controller_->GetLastError();
        if (!err.empty()) {
            setError(err);
        }
    }
}

std::vector<SDRConfig> USRPDevice::detectUSRPDevices() {
    std::vector<SDRConfig> configs;
    // TODO: Use UHD's device discovery API
    try {
        SDRConfig config;
        config.device_type = "usrp";
        config.serial_number = "32C1EC6";
        config.frequency = 2.45e9;
        config.sample_rate = 10e6;
        config.gain = 40.0;
        configs.push_back(config);
        
    } catch (const std::exception& e) {
        std::cerr << "Error detecting USRP devices: " << e.what() << std::endl;
    }
    return configs;
}
