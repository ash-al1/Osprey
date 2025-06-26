#include "UsrpController.h"
#include <iostream>
#include <iomanip>

// Basic checks
void TestBasics(UsrpController& usrp) {
	std::cout << "Testing basic functions..." << std::endl;
	std::cout << "Serial number: " << usrp.GetSerialNumber() << std::endl;
	std::cout << "Master clock rate: ..." << usrp.GetMasterClockRate() << std::endl;
	std::cout << "100 Mhz frequency check: " << (usrp.IsFrequencyValid(100e6) ? "Yes" : "No") << std::endl;	
	std::cout << "1 Mhz frequency check: " << (usrp.IsFrequencyValid(1e6) ? "Yes" : "No") << std::endl;	
	std::cout << "1MS/s check: " << (usrp.IsSampleRateValid(1e6) ? "Yes" : "No") << std::endl;	
	std::cout << "20dB Rx gain check: " << (usrp.IsRxGainValid(20) ? "Yes" : "No") << std::endl;	
	std::cout << "100dB Rx gain check: " << (usrp.IsRxGainValid(100) ? "Yes" : "No") << std::endl;	
}

// Setters & getters checks 
void TestParameters(UsrpController& usrp) {
	std::cout << "1. Testing setting parameters of usrp..." << std::endl;
	std::cout << "Set Rx center freq to 915Mhz" << std::endl;
	if (usrp.SetRxFrequency(915e6)) {
		double actual_freq = usrp.GetRxFrequency();
		std::cout << " SUCCESS: Actual frequency = " << actual_freq / 1e6 << "Hz" << std::endl;
	} else {
		std:: cout << " FAILED: " << usrp.GetLastError() << std::endl;
	}
	std::cout << "Set Rx sample rate to 2MS/s" << std::endl;
	if (usrp.SetRxSampleRate(2e6)) {
		double actual_rate = usrp.GetRxSampleRate();
		std::cout << " SUCCESS: Actual rate = " << actual_rate / 1e6 << "MS/s" << std::endl;
	} else {
		std:: cout << " FAILED: " << usrp.GetLastError() << std::endl;
	}
	std::cout << "Set Rx gain to 30dB" << std::endl;
	if (usrp.SetRxGain(30)) {
		double actual_gain = usrp.GetRxGain();
		std::cout << " SUCCESS: Actual gain = " << actual_gain << "dB" << std::endl;
	} else {
		std:: cout << " FAILED: " << usrp.GetLastError() << std::endl;
	}
	std::cout << "Set Rx bandwidth to 1.5Mhz" << std::endl;
	if (usrp.SetRxBandwidth(30)) {
		double actual_bw = usrp.GetRxBandwidth();
		std::cout << " SUCCESS: Actual bandiwdth = " << actual_bw / 1e6 << "Mhz" << std::endl;
	} else {
		std:: cout << " FAILED: " << usrp.GetLastError() << std::endl;
	}
}

void TestErrors(UsrpController& usrp) {
	std::cout << "2. Testing errors..." << std::endl;
	std::cout << "Freq check (1Mhz)" << std::endl;
	if (!usrp.SetRxFrequency(1e6)) {
		std::cout << " Expected failure: " << usrp.GetLastError() << std::endl;
	} else {
		std::cout << " Incorrect setting of frequency " << std::endl;
	}
	std::cout << "Gain check (100dB)" << std::endl;
	if (!usrp.SetRxGain(100)) {
		std::cout << " Expected failure: " << usrp.GetLastError() << std::endl;
	} else {
		std::cout << " Incorrect setting of gain " << std::endl;
	}
	std::cout << "Rate check (100MS/s)" << std::endl;
	if (!usrp.SetRxSampleRate(100e6)) {
		std::cout << " Expected failure: " << usrp.GetLastError() << std::endl;
	} else {
		std::cout << " Incorrect setting of sample rate " << std::endl;
	}
}

void PrintGetters(UsrpController& usrp) { 
	std::cout << "3. Getters..." << std::endl;
	std::cout << std::fixed << std::setprecision(3);
	std::cout << "Rx freq: " << usrp.GetRxFrequency() / 1e6 << std::endl;
	std::cout << "Rx rate: " << usrp.GetRxSampleRate() / 1e6 << std::endl;
	std::cout << "Rx gain: " << usrp.GetRxGain() << std::endl;
	std::cout << "Rx bandwidth: " << usrp.GetRxBandwidth() / 1e6 << std::endl;
	std::cout << "Rx antenna: " << usrp.GetRxAntenna() << std::endl;
}	

int main() {
	std::cout << "USRP B210 Controller Test Program" << std::endl;
	std::cout << "=================================" << std::endl;

	UsrpController usrp;

	std::cout << "Attempting to connect to B210" << std::endl;
	if (!usrp.Initialize("32C1EC6")) {
		std::cerr << "Failed to initialize: " << usrp.GetLastError() << std::endl;
		return -1;
	}
	std::cout << "SUCCESS: USRP initialized" << std::endl;

	try {
		TestBasics(usrp);
		TestParameters(usrp);
		TestErrors(usrp);
		PrintGetters(usrp);
	} catch(std::exception& e) {
		std::cerr << "Exception during testing : " <<  e.what() << std::endl;
		return -1;
	}

	std::cout << "\nShutting down USRP" << std::endl;
	return 0;
}

