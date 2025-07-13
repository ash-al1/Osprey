#pragma once

#include "SDRDevice.h"
#include <memory>
#include <map>
#include <functional>
#include <vector>

class SDRFactory {
public:
	using DeviceCreator = std::function<std::unique_ptr<SDRDevice>()>;

	/**
     * Create an SDR device instance
     * @param device_type Device type string (e.g., "usrp", "rtlsdr")
     * @return Unique pointer to device instance, nullptr if type not supported
     */
	static std::unique_ptr<SDRDevice> create(const std::string& device_type);

	/**
     * Create and initialize an SDR device
     * @param config Complete device configuration
     * @return Initialized device instance, nullptr on failure
     */
	static std::unique_ptr<SDRDevice> createAndInitialize(const SDRConfig& confg);

	/**
     * Register a device type with its creator function
     * @param device_type Device type identifier
     * @param creator Function that creates device instances
     * @return true if registered successfully
     */
    static bool registerDevice(const std::string& device_type, DeviceCreator creator);

	/**
     * Get list of supported device types
     * @return Vector of registered device type strings
     */
    static std::vector<std::string> getSupportedDevices();

	/**
     * Check if a device type is supported
     * @param device_type Device type to check
     * @return true if device type is registered
     */
    static bool isDeviceSupported(const std::string& device_type);

	/**
     * Auto-detect connected devices
     * @return Vector of detected device configurations
     */
    static std::vector<SDRConfig> detectDevices();

private:
    static std::map<std::string, DeviceCreator>& getRegistry();
};

/**
 * Helper class for automatic device registration
 * Usage: static SDRDeviceRegistrar<MyDevice> my_device_reg("mydevice");
 */
template<typename T>
class SDRDeviceRegistrar {
public:
    explicit SDRDeviceRegistrar(const std::string& device_type) {
        SDRFactory::registerDevice(device_type, []() {
            return std::make_unique<T>();
        });
    }
};
