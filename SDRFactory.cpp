#include "SDRFactory.h"
#include <iostream>
#include <algorithm>

std::map<std::string, SDRFactory::DeviceCreator>& SDRFactory::getRegistry() {
    static std::map<std::string, DeviceCreator> registry;
    return registry;
}

std::unique_ptr<SDRDevice> SDRFactory::create(const std::string& device_type) {
    auto& registry = getRegistry();
    std::string lower_type = device_type;
    std::transform(lower_type.begin(), lower_type.end(), lower_type.begin(), ::tolower);
    auto it = registry.find(lower_type);
    if (it != registry.end()) {
        try {
            return it->second();
        } catch (const std::exception& e) {
            std::cerr << "Failed to create device '" << device_type 
                      << "': " << e.what() << std::endl;
            return nullptr;
        }
    }
    std::cerr << "Unknown device type: " << device_type << std::endl;
    std::cerr << "Supported devices: ";
    for (const auto& dev : getSupportedDevices()) {
        std::cerr << dev << " ";
    }
    std::cerr << std::endl;
    
    return nullptr;
}

std::unique_ptr<SDRDevice> SDRFactory::createAndInitialize(const SDRConfig& config) {
    auto device = create(config.device_type);
    if (!device) {
        return nullptr;
    }
    if (!device->initialize(config)) {
        std::cerr << "Failed to initialize " << config.device_type 
                  << ": " << device->getLastError() << std::endl;
        return nullptr;
    }
    
    bool success = true;
    
    if (config.frequency > 0) {
        success &= device->setFrequency(config.frequency, config.channel);
    }
    if (config.sample_rate > 0) {
        success &= device->setSampleRate(config.sample_rate, config.channel);
    }
    if (config.gain >= 0) {
        success &= device->setGain(config.gain, config.channel);
    }
    if (config.bandwidth > 0) {
        success &= device->setBandwidth(config.bandwidth, config.channel);
    }
    if (!config.antenna.empty()) {
        success &= device->setAntenna(config.antenna, config.channel);
    }
    if (!success) {
        std::cerr << "Warning: Some parameters could not be set: " 
                  << device->getLastError() << std::endl;
    }
    return device;
}

bool SDRFactory::registerDevice(const std::string& device_type, DeviceCreator creator) {
    if (!creator) {
        std::cerr << "Cannot register device with null creator" << std::endl;
        return false;
    }
    std::string lower_type = device_type;
    std::transform(lower_type.begin(), lower_type.end(), lower_type.begin(), ::tolower);

    auto& registry = getRegistry();

    if (registry.find(lower_type) != registry.end()) {
        std::cerr << "Device type '" << device_type << "' already registered" << std::endl;
        return false;
    }
    registry[lower_type] = creator;
    std::cout << "Registered SDR device type: " << device_type << std::endl;
    return true;
}

std::vector<std::string> SDRFactory::getSupportedDevices() {
    std::vector<std::string> devices;
    const auto& registry = getRegistry();

    devices.reserve(registry.size());
    for (const auto& [type, creator] : registry) {
        devices.push_back(type);
    }

    return devices;
}

bool SDRFactory::isDeviceSupported(const std::string& device_type) {
    std::string lower_type = device_type;
    std::transform(lower_type.begin(), lower_type.end(), lower_type.begin(), ::tolower);
    const auto& registry = getRegistry();
    return registry.find(lower_type) != registry.end();
}

std::vector<SDRConfig> SDRFactory::detectDevices() {
    std::vector<SDRConfig> detected;
    std::cout << "Auto-detecting SDR devices..." << std::endl;
    // TODO: Each device should implement a static detect()
    return detected;
}
