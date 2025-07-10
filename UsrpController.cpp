/*
 * References:
 * https://github.com/EttusResearch/uhd/blob/master/host/examples/rx_samples_to_file.cpp
 */

#include "UsrpController.h"
#include <uhd/utils/thread.hpp>
#include <uhd/utils/safe_main.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <chrono>

// Usrp Object
UsrpController::UsrpController()
	: usrp_device_(nullptr)
	, serial_number_("")
	, initialized_(false)
	, last_error_("")
	, receiving_(false)
	, stop_receiving_(false)
	, receive_thread_(nullptr)
	, buffer_size_(4096)
	, total_samples_received_(0)
	, overflow_count_(0) {
}

UsrpController::~UsrpController() {
	StopReceiving();
	Shutdown();
}

/************************************CORE**************************************/

bool UsrpController::Initialize(const std::string& serial_number) {
	// Wrap in try-catch for error-handling to prevent malfunction & crashes
	try {
		if (initialized_) {
			SetError("USRP device is busy");
			return false;
		}
		serial_number_ = serial_number;
		std::string device_args = "serial=" + serial_number;
		std::cout << "Attempting to connect to USRP B210 with serial: " << serial_number << std::endl;
		usrp_device_ = uhd::usrp::multi_usrp::make(device_args);

		if (!usrp_device_) { 
			SetError("Failed to create USRP device");
			return false;
		}

		// Check initialization
		initialized_ = true;
		std::cout << "Successfully connected to USRP" << std::endl;

		SetClockSource("internal");
		SetTimeSource("internal");

		// Wait for lock
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		std::cout << "Device info: " << GetDeviceInfo() << std::endl;

		// Defaults to prevent fuck ups
		SetRxFrequency(100e6);
		SetRxSampleRate(1e6);
		SetRxGain(20.0);

		SetError("");
		return true;
	} catch (const uhd::exception& e) {
		SetError("UHD Exception during init: " + std::string(e.what()));
		usrp_device_.reset();
		return false;
	} catch (const std::exception& e) {
		SetError("STD Exception during init: " + std::string(e.what()));
		usrp_device_.reset();
		return false;
	}
}

void UsrpController::Shutdown() {
	if (usrp_device_) {
		try {
			std::cout << "Shuting down USRP ..." << std::endl;
			usrp_device_.reset();
		} catch (const std::exception& e) {
			std::cerr << "(BAD) Error during shutdown: " << e.what() << std::endl;
		}
	}
	initialized_ = false;
	serial_number_.clear();
	SetError("");
}

std::string UsrpController::GetDeviceInfo() const {
	if (!ValidateDevice()) return "";
	try {
		auto tree = usrp_device_->get_device()->get_tree();
		std::stringstream ss;
		ss << usrp_device_->get_pp_string();
		return ss.str();
	} catch (const std::exception& e) {
		SetError("Error getting device information: " + std::string(e.what()));
		return "";
	}
}

std::string UsrpController::GetSerialNumber() const {
	return serial_number_;
}

double UsrpController::GetMasterClockRate() const {
	if (!ValidateDevice()) return 0.0;
	try {
		return usrp_device_->get_master_clock_rate();
	} catch(const std::exception& e) {
		SetError("Error getting master clock rate: " + std::string(e.what()));
		return 0.0;
	}
}

bool UsrpController::SyncClock() {
	if (!ValidateDevice()) return false;
	try {
		usrp_device_->set_time_now(uhd::time_spec_t(0.0));
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		std::cout << "Clock synched" << std::endl;
		return true;
	} catch (const std::exception& e) {
		SetError("Error synchronizing clock: " + std::string(e.what()));
		return false;
	}
}

/***********************************SETTER*************************************/

bool UsrpController::SetClockSource(const std::string& clock_source) {
	if (!ValidateDevice()) return false;
	try {
		usrp_device_->set_clock_source(clock_source);
		if (clock_source != "internal") {
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			std::cout << "Clock source not internal (NOT CAUGHT)" << std::endl;
		}
		return true;
	} catch(const std::exception& e) {
		SetError("Error setting clock: " + std::string(e.what()));
		return false;
	}
}

bool UsrpController::SetTimeSource(const std::string& time_source) {
	if (!ValidateDevice()) return false;
	try {
		usrp_device_->set_time_source(time_source);
		return true;
	} catch(std::exception& e) {
		SetError("Error setting time: " + std::string(e.what()));
		return false;
	}
}

bool UsrpController::SetRxFrequency(double freq_hz, size_t channel) {
	if (!ValidateDevice() || !ValidateChannel(channel)) return false;
	if (!IsFrequencyValid(freq_hz)) {
		SetError("Invalid Rx frequency: "+ std::to_string(freq_hz) + "Hz");
		return false;
	}
	try {
		uhd::tune_request_t tune_request(freq_hz);
		usrp_device_->set_rx_freq(tune_request, channel);
		double actual_freq = usrp_device_->get_rx_freq(channel);
		// Offset threshold, may need tuning
		if (std::abs(actual_freq - freq_hz > 1e3)) {
			SetError("Requested frequency" + std::to_string(freq_hz) + ", Actual: " + std::to_string(actual_freq));
			return false;
		}
		return true;
	} catch(const std::exception& e) {
		SetError("Error setting Rx frequency: " + std::string(e.what()));
		return false;
	}
}

bool UsrpController::SetRxSampleRate(double rate_sps, size_t channel) {
	if (!ValidateDevice() || !ValidateChannel(channel)) return false;
	if (!IsSampleRateValid(rate_sps)) {
		SetError("Invalid sample rate: " + std::to_string(rate_sps) + "S/s");
		return false;
	}
	try {
		usrp_device_->set_rx_rate(rate_sps, channel);
		double actual_rate = usrp_device_->get_rx_rate(channel);
		// Check for any offset
		if(std::abs(actual_rate - rate_sps) / rate_sps > 0.01) {
			std::cout << "Offset: Rx rate set to " << actual_rate << " instead of  " << rate_sps << std::endl;
		}
		return true;
	} catch(const std::exception& e) {
		SetError("Error setting sample rate: " + std::string(e.what()));
		return false;
	}
}

bool UsrpController::SetRxBandwidth(double bandwidth_hz, size_t channel) {
	if (!ValidateDevice() || !ValidateChannel(channel)) return false;
	if (bandwidth_hz <= 0 || bandwidth_hz > MAX_SAMPLE_RATE) {
		SetError("Invalid Rx bandwidth: " + std::to_string(bandwidth_hz) + "Hz");
		return false;
	}
	try {
		usrp_device_->set_rx_bandwidth(bandwidth_hz, channel);
		return true;
	} catch(const std::exception& e) {
		SetError("Error setting Rx bandwidth: " + std::string(e.what()));
		return false;
	}
}

bool UsrpController::SetRxGain(double gain_db, size_t channel) {
	if (!ValidateDevice() || !ValidateChannel(channel)) return false;
	if (!IsRxGainValid(gain_db)) {
		// Documentation 'coerces' gain to correct value but lets just throw an error
		SetError("Invalid Rx gain: " + std::to_string(gain_db) + "dB");
		return false;
	}
	try {
		usrp_device_->set_rx_gain(gain_db, channel);
		return true;
	} catch(const std::exception& e) {
		SetError("Error setting Rx gain: " + std::string(e.what()));
		return false;
	}
}

bool UsrpController::SetRxAntenna(const std::string& antenna, size_t channel) {
	if (!ValidateDevice() || !ValidateChannel(channel)) return false;
	try {
		usrp_device_->set_rx_antenna(antenna, channel);
		return true;
	} catch(const std::exception& e) {
		SetError("Error setting Rx antenna " + std::string(e.what()));
		return false;
	}
}

/***********************************GETTER*************************************/

double UsrpController::GetRxFrequency(size_t channel) const {
	if (!ValidateDevice() || !ValidateChannel(channel)) return 0.0;
	try {
		return usrp_device_->get_rx_freq(channel);
	} catch(const std::exception& e) {
		SetError("Error getting Rx frequency: " + std::string(e.what()));
		return 0.0;
	}
}

double UsrpController::GetRxSampleRate(size_t channel) const {
	if (!ValidateDevice() || !ValidateChannel(channel)) return 0.0;
	try {
		return usrp_device_->get_rx_rate(channel);
	} catch(const std::exception& e) {
		SetError("Error getting Rx sample rate: " + std::string(e.what()));
		return 0.0;
	}
}

double UsrpController::GetRxBandwidth(size_t channel) const {
	if (!ValidateDevice() || !ValidateChannel(channel)) return 0.0;
	try {
		return usrp_device_->get_rx_bandwidth(channel);
	} catch(const std::exception& e) {
		SetError("Error getting Rx bandwidth: " + std::string(e.what()));
		return 0.0;
	}
}

double UsrpController::GetRxGain(size_t channel) const {
	if (!ValidateDevice() || !ValidateChannel(channel)) return 0.0;
	try {
		return usrp_device_->get_rx_gain(channel);
	} catch(const std::exception& e) {
		SetError("Error getting Rx gain: " + std::string(e.what()));
		return 0.0;
	}
}

std::string UsrpController::GetRxAntenna(size_t channel) const {
	if (!ValidateDevice() || !ValidateChannel(channel)) return "";
	try {
		return usrp_device_->get_rx_antenna(channel);
	} catch(const std::exception& e) {
		SetError("Error getting Rx antenna: " + std::string(e.what()));
		return "";
	}
}

/**********************************IS_VALID************************************/

bool UsrpController::IsFrequencyValid(double freq_hz) const {
	return freq_hz >= MIN_FREQUENCY_HZ && freq_hz <= MAX_FREQUENCY_HZ;
}

bool UsrpController::IsSampleRateValid(double rate_sps) const {
	return rate_sps >= MIN_SAMPLE_RATE && rate_sps <= MAX_SAMPLE_RATE;
}

bool UsrpController::IsRxGainValid(double gain_db) const {
	return gain_db >= MIN_RX_GAIN_DB && gain_db <= MAX_RX_GAIN_DB;
}

/***********************************HELPER*************************************/

void UsrpController::SetError(const std::string& error) const {
	last_error_ = error;
	if (!error.empty()) {
		std::cerr << "USRP Error: " << error << std::endl;
	}
}

bool UsrpController::ValidateChannel(size_t channel) const {
	if (channel >= usrp_device_->get_rx_num_channels()) {
		SetError("Invalid channel: " + std::to_string(channel));
		return false;
	}
	return true;
}

bool UsrpController::ValidateDevice() const {
	if (!initialized_ || !usrp_device_) {
		SetError("USRP not initialized");
		return false;
	}
	return true;
}

/**************************************RX**************************************/

bool UsrpController::StartReceiving(SampleCallback callback,size_t buffer_size){
	if (!ValidateDevice()) return false;
	if (receiving_.load()) {
		SetError("Already receiving");
		return false;
	}
	sample_callback_ = callback;
	buffer_size_ = buffer_size;
	stop_receiving_.store(false);
	total_samples_received_.store(0);
	overflow_count_.store(0);
	try {
		receive_thread_ = std::make_unique<std::thread>(
				&UsrpController::ReceiveWorker, this);
		receiving_.store(true);
		std::cout << "Started receiving samples" << std::endl;
		return true;
	} catch (const std::exception& e) {
		SetError("Failed to start receive thread: " + std::string(e.what()));
		return false;
	}
}

void UsrpController::StopReceiving() {
	if (!receiving_.load()) return;
	std::cout << "Stopping sample reception..." << std::endl;
	stop_receiving_.store(true);
	if (receive_thread_ && receive_thread_->joinable()) {
		receive_thread_->join();
	}
	receiving_.store(false);
	receive_thread_.reset();
	std::cout << "Total samples (RX): " << total_samples_received_.load()
		<< "\noverflows: " << overflow_count_.load() << std::endl;
}

void UsrpController::ReceiveWorker() {
	try {
		uhd::stream_args_t stream_args("fc32", "sc16");
		stream_args.channels = {0};
		auto rx_stream = usrp_device_->get_rx_stream(stream_args);
		std::vector<std::complex<float>> buffer(buffer_size_);
		uhd::rx_metadata_t md;
		uhd::stream_cmd_t stream_cmd(
				uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
		stream_cmd.stream_now = true;
		rx_stream->issue_stream_cmd(stream_cmd);
		std::cout << "Rx worker start" << std::endl;
		while (!stop_receiving_.load()) {
			size_t num_rx_samps = rx_stream->recv(&buffer.front(),
					buffer_size_, md, 1.0);
			// Timeouts may or may not be okay, who the fuck knows
			if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
				continue;
			}
			if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW) {
				overflow_count_.fetch_add(1);
				std::cout << "Overflow count: " << overflow_count_.load() << 
					std::endl;
				continue;
			}
			if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
                std::cout << "Debug: error_code = " << (int)md.error_code
                         << ", strerror = '" << md.strerror() << "'" << std::endl;

                SetError("Receive error code " + std::to_string((int)md.error_code) + ": " + md.strerror());
                continue;
            }
			if (sample_callback_ && num_rx_samps > 0) {
				sample_callback_(buffer.data(), num_rx_samps);
				total_samples_received_.fetch_add(num_rx_samps);
			}
		}
		stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
		rx_stream->issue_stream_cmd(stream_cmd);
	} catch (const std::exception& e) {
		SetError("Receive thread error" + std::string(e.what()));
	}
	std::cout << "Rx worker done" << std::endl;
}
