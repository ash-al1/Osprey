#pragma once

#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/exception.hpp>
#include <string>
#include <memory>

class UsrpController {
public:
	UsrpController();
	~UsrpController();

	// Core functions
	bool Initialize(const std::string& serial_number="32C1EC6");
	bool IsInitialized() const { return usrp_device_ != nullptr; }
	void Shutdown();

	// Device info
	std::string GetDeviceInfo() const;
	std::string GetSerialNumber() const;
	double GetMasterClockRate() const;

	// Clock sync
	bool SyncClock();
	bool SetClockSource(const std::string& clock_source = "internal");
	bool SetTimeSource(const std::string& time_source = "internal");

	// RX config
	bool SetRxFrequency(double freq_hz, size_t channel = 0);
	bool SetRxSampleRate(double rate_sps, size_t channel = 0);
	bool SetRxBandwidth(double bandwidth_hz, size_t channel = 0);
	bool SetRxGain(double gain_db, size_t channel = 0);
	bool SetRxAntenna(const std::string& antenna = "TX/RX", size_t channel = 0);

	// Getters
	double GetRxFrequency(size_t channel = 0) const;
	double GetRxSampleRate(size_t channel = 0) const;
	double GetRxBandwidth(size_t channel = 0) const;
	double GetRxGain(size_t channel = 0) const;
	std::string GetRxAntenna(size_t channel = 0) const;

	// Asserts
	bool IsFrequencyValid(double freq_hz) const;	
	bool IsSampleRateValid(double rate_sps) const;	
	bool IsRxGainValid(double gain_db) const;	

	// Errors
	std::string GetLastError() const { return last_error_; }

private:
	// Usrp device
	uhd::usrp::multi_usrp::sptr usrp_device_;
	std::string serial_number_;
	bool initialized_;

	// Error tracker
	mutable std::string last_error_;

	// Helpers
	void SetError(const std::string& error) const;
	bool ValidateChannel(size_t channel) const;
	bool ValidateDevice() const;

	// Usrp Parameters (NI2901 specific)
	static constexpr double MIN_FREQUENCY_HZ = 70e6;
	static constexpr double MAX_FREQUENCY_HZ = 6e9;
	static constexpr double MIN_SAMPLE_RATE = 200e3;
	static constexpr double MAX_SAMPLE_RATE = 61.44e6;
	static constexpr double MIN_RX_GAIN_DB = 0.0;
	static constexpr double MAX_RX_GAIN_DB = 76.0;
};
