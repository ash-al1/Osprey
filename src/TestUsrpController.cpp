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

void TestRx(UsrpController& usrp) {
	std::cout << "4. Testing on 2.4GHz single reception..." << std::endl;
	if (usrp.SetRxFrequency(2.45e9)) {
		std::cout << "	freq: " << usrp.GetRxFrequency() / 1e9 << std::endl;
	} else {
		std::cout << " Failed to set freq" << usrp.GetLastError() << std::endl;
		return;
	}
	if (usrp.SetRxSampleRate(10e6)) {
		std::cout << "	sample rate: " << usrp.GetRxSampleRate() / 1e6
			<< std::endl;
	} else {
		std::cout << " Failed to set sample rate" << usrp.GetLastError()
			<< std::endl;
		return;
	}
	if (usrp.SetRxGain(40.0)) {
		std::cout << "	gain: " << usrp.GetRxGain() << std::endl;
	} else {
		std::cout << " Failed to set gain" << usrp.GetLastError() << std::endl;
		return;
	}
	if (usrp.SetRxBandwidth(12e6)) {
		std::cout << "	bw: " << usrp.GetRxBandwidth() << std::endl;
	} else {
		std::cout << " Failed to bandwidth" << usrp.GetLastError() << std::endl;
		return;
	}

	std::cout << "starting on 2.4GHz..." << std::endl;
	size_t samples_received = 0;
	float max_magnitude = 0.0f;
	float avg_magnitude = 0.0f;

	auto callback = [&](const std::complex<float>* samples, size_t count) {
        samples_received += count;
        
        float batch_sum = 0.0f;
        float batch_max = 0.0f;
        for (size_t i = 0; i < count; ++i) {
            float magnitude = std::abs(samples[i]);
            batch_sum += magnitude;
            if (magnitude > batch_max) {
                batch_max = magnitude;
            }
        }
        
        avg_magnitude = (avg_magnitude * (samples_received - count) +
				batch_sum) / samples_received;
        if (batch_max > max_magnitude) {
            max_magnitude = batch_max;
        }
        
        if (samples_received % 1000000 == 0) {
            std::cout << "  Received " << samples_received / 1000000
				<< "M samples..." << std::endl;
        }
    };

	if (!usrp.StartReceiving(callback, 8192)) {
        std::cout << "  FAILED to start receiving: "
			<< usrp.GetLastError() << std::endl;
        return;
    }

    std::cout << "  reception started" << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(5));
    usrp.StopReceiving();

    std::cout << "  test completed:" << std::endl;
    std::cout << "    total samples received: "
		<< usrp.GetTotalSamplesReceived() << std::endl;
    std::cout << "    overflow count: " << usrp.GetOverflowCount() << std::endl;
    std::cout << "    max signal magnitude: " << max_magnitude << std::endl;
    std::cout << "    average signal magnitude: " << avg_magnitude << std::endl;

    double expected_samples = usrp.GetRxSampleRate() * 5.0;
    double reception_rate = (double)usrp.GetTotalSamplesReceived() / expected_samples * 100.0;
    std::cout << "    Rx rate: " << std::fixed << std::setprecision(1)
              << reception_rate << "%" << std::endl;

    if (usrp.GetOverflowCount() == 0 && reception_rate > 95.0) {
        std::cout << "  SUCCESS" << std::endl;
    } else if (usrp.GetOverflowCount() > 10) {
        std::cout << "  WARNING: High overflow count" << std::endl;
    } else {
        std::cout << "  WARNING: High underflow count" << std::endl;
    }

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
		TestRx(usrp);
		/*TestBasics(usrp);
		TestParameters(usrp);
		TestErrors(usrp);
		PrintGetters(usrp);*/
	} catch(std::exception& e) {
		std::cerr << "Exception during testing : " <<  e.what() << std::endl;
		return -1;
	}

	std::cout << "\nShutting down USRP" << std::endl;
	return 0;
}

