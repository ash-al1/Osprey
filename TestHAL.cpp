#include "SDRFactory.h"
#include "SDRDevice.h"
#include "USRPDevice.h"
#include "SimulationDevice.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <atomic>

std::atomic<size_t> g_sample_count{0};
std::atomic<bool> g_stop_flag{false};

void sample_callback(const std::complex<float>* samples, size_t count) {
    g_sample_count.fetch_add(count);
    
    // Debug first few samples
    static int print_count = 0;
    if (print_count++ < 5) {
        std::cout << "Received " << count << " samples. First sample: " 
                  << samples[0].real() << " + " << samples[0].imag() << "i" << std::endl;
    }
}

void test_device_creation() {
    std::cout << "\n=== Testing Device Creation ===" << std::endl;
    
    // List supported devices
    auto supported = SDRFactory::getSupportedDevices();
    std::cout << "Supported devices: ";
    for (const auto& dev : supported) {
        std::cout << dev << " ";
    }
    std::cout << std::endl;
    
    // Test creating simulation device
    auto sim_device = SDRFactory::create("simulation");
    if (sim_device) {
        std::cout << "  Created simulation device" << std::endl;
        std::cout << "  Type: " << sim_device->getDeviceType() << std::endl;
        std::cout << "  Serial: " << sim_device->getSerialNumber() << std::endl;
    } else {
        std::cout << "  Failed to create simulation device" << std::endl;
    }
    
    // Test USRP init
    auto usrp_device = SDRFactory::create("usrp");
    if (usrp_device) {
        std::cout << "  Created USRP device instance" << std::endl;
    } else {
        std::cout << "  Failed to create USRP device instance" << std::endl;
    }
    
    // Test invalid device type
    auto invalid_device = SDRFactory::create("invalid_device");
    if (!invalid_device) {
        std::cout << "  Correctly rejected invalid device type" << std::endl;
    } else {
        std::cout << "  Should have rejected invalid device type" << std::endl;
    }
}

void test_simulation_device() {
    std::cout << "\n=== Testing Simulation Device ===" << std::endl;
    
    SDRConfig config;
    config.device_type = "simulation";
    config.frequency = 100e6;
    config.sample_rate = 1e6;
    config.gain = 20.0;
    
    auto device = SDRFactory::createAndInitialize(config);
    if (!device) {
        std::cout << "  Failed to create and initialize simulation device" << std::endl;
        return;
    }
    
    std::cout << "  Initialized simulation device" << std::endl;
    
    // Check capabilities
    auto caps = device->getCapabilities();
    std::cout << "Capabilities:" << std::endl;
    std::cout << "  Frequency range: " << caps.min_frequency / 1e6 << " - " 
              << caps.max_frequency / 1e9 << " GHz" << std::endl;
    std::cout << "  Sample rate range: " << caps.min_sample_rate / 1e3 << " kS/s - " 
              << caps.max_sample_rate / 1e6 << " MS/s" << std::endl;
    std::cout << "  Gain range: " << caps.min_gain << " - " << caps.max_gain << " dB" << std::endl;
    
    // Test parameter setting
    if (device->setFrequency(433e6)) {
        std::cout << "  Set frequency to 433 MHz" << std::endl;
    }
    
    if (device->setSampleRate(2e6)) {
        std::cout << "  Set sample rate to 2 MS/s" << std::endl;
    }
    
    if (device->setGain(30.0)) {
        std::cout << "  Set gain to 30 dB" << std::endl;
    }
    
    // Check status
    auto status = device->getStatus();
    std::cout << "Device status:" << std::endl;
    std::cout << "  Initialized: " << (status.initialized ? "Yes" : "No") << std::endl;
    std::cout << "  Frequency: " << status.current_frequency / 1e6 << " MHz" << std::endl;
    std::cout << "  Sample rate: " << status.current_sample_rate / 1e6 << " MS/s" << std::endl;
    std::cout << "  Gain: " << status.current_gain << " dB" << std::endl;
    
    std::cout << "\nTesting sample reception for 3 seconds..." << std::endl;
    g_sample_count.store(0);
    
    if (!device->startReceiving(sample_callback, 4096)) {
        std::cout << "✗ Failed to start receiving" << std::endl;
        return;
    }
    std::cout << "  Started receiving" << std::endl;
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
    device->stopReceiving();
    
    size_t total_samples = g_sample_count.load();
    double expected_samples = status.current_sample_rate * 3.0;
    double reception_rate = (total_samples / expected_samples) * 100.0;
    
    std::cout << "Reception complete:" << std::endl;
    std::cout << "  Total samples: " << total_samples << std::endl;
    std::cout << "  Expected: " << static_cast<size_t>(expected_samples) << std::endl;
    std::cout << "  Reception rate: " << std::fixed << std::setprecision(1) 
              << reception_rate << "%" << std::endl;
    std::cout << "  Device overflow count: " << device->getOverflowCount() << std::endl;
    
    if (reception_rate > 95.0 && device->getOverflowCount() == 0) {
        std::cout << "  Sample reception test PASSED" << std::endl;
    } else {
        std::cout << "  Sample reception test FAILED" << std::endl;
    }
}

void test_usrp_device_creation() {
    std::cout << "\n=== Testing USRP Device Creation ===" << std::endl;
    
	// Hardcoded test for my given USRP
    SDRConfig config;
    config.device_type = "usrp";
    config.serial_number = "32C1EC6";
    config.frequency = 2.45e9;
    config.sample_rate = 10e6;
    config.gain = 40.0;
    
    auto device = SDRFactory::create("usrp");
    if (!device) {
        std::cout << "  Failed to create USRP device instance" << std::endl;
        return;
    }
    
    std::cout << "  Created USRP device instance" << std::endl;
    std::cout << "  Type: " << device->getDeviceType() << std::endl;
    
    // Try to initialize
    bool init_success = device->initialize(config);
    if (!init_success) {
        std::cout << "  USRP initialization failed as expected without hardware" << std::endl;
        std::cout << "  Error: " << device->getLastError() << std::endl;
    } else {
        std::cout << "  USRP device initialized (hardware found!)" << std::endl;
        
        // If we have hardware, run a quick test
        auto caps = device->getCapabilities();
        std::cout << "USRP Capabilities:" << std::endl;
        std::cout << "  Frequency range: " << caps.min_frequency / 1e6 << " MHz - " 
                  << caps.max_frequency / 1e9 << " GHz" << std::endl;
        std::cout << "  Max sample rate: " << caps.max_sample_rate / 1e6 << " MS/s" << std::endl;
        std::cout << "  Gain range: " << caps.min_gain << " - " << caps.max_gain << " dB" << std::endl;
    }
}

void test_device_polymorphism() {
    std::cout << "\n=== Testing Device Polymorphism ===" << std::endl;
    
    std::vector<std::unique_ptr<SDRDevice>> devices;
    
    // Add simulation device
    SDRConfig sim_config;
    sim_config.device_type = "simulation";
    auto sim = SDRFactory::createAndInitialize(sim_config);
    if (sim) devices.push_back(std::move(sim));
    
    SDRConfig usrp_config;
    usrp_config.device_type = "usrp";
    usrp_config.serial_number = "32C1EC6";
    auto usrp = SDRFactory::createAndInitialize(usrp_config);
    if (usrp) devices.push_back(std::move(usrp));
    
    std::cout << "Created " << devices.size() << " device(s)" << std::endl;
    
    for (const auto& device : devices) {
        std::cout << "\nDevice: " << device->getDeviceType() << std::endl;
        std::cout << "  Serial: " << device->getSerialNumber() << std::endl;
        
        auto caps = device->getCapabilities();
        std::cout << "  Channels: " << caps.num_channels << std::endl;
        std::cout << "  Has bandwidth control: " << (caps.has_adjustable_bandwidth ? "Yes" : "No") << std::endl;
        
        if (device->setFrequency(100e6)) {
            std::cout << "    Set frequency to 100 MHz" << std::endl;
        }
    }
}

int main() {
    std::cout << "=== SDR HAL Architecture Test Suite ===" << std::endl;
    std::cout << "Testing the new Hardware Abstraction Layer..." << std::endl;
    
    try {
        test_device_creation();
        test_simulation_device();
        test_usrp_device_creation();
        test_device_polymorphism();
        
        std::cout << "\n=== All Tests Complete ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}
