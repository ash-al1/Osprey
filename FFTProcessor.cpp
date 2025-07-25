#include "FFTProcessor.h"
#include <pffft.h>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <cstring>

FFTProcessor::FFTProcessor(int fft_size) 
    : setup_(nullptr)
    , fft_size_(fft_size) {
    
    // Initialize PFFFT for real to complex forward FFT
    setup_ = pffft_new_setup(fft_size, PFFFT_REAL);
    if (!setup_) {
        throw std::runtime_error("Failed to create PFFFT setup for size " + std::to_string(fft_size));
    }
    
    // Allocate work buffer
    work_buffer_.resize(fft_size);
    
    std::cout << "FFTProcessor initialized with PFFFT, size=" << fft_size << std::endl;
}

FFTProcessor::~FFTProcessor() {
    cleanup();
}

void FFTProcessor::cleanup() {
    if (setup_) {
        pffft_destroy_setup(setup_);
        setup_ = nullptr;
    }
}

void FFTProcessor::forwardFFT(const float* input_buffer, float* output_buffer) {
    // Perform the forward FFT, must be ordered for the result to make sense
    pffft_transform_ordered(setup_, 
                           input_buffer, 
                           output_buffer, 
                           work_buffer_.data(), 
                           PFFFT_FORWARD);
}

void FFTProcessor::complexToReal(float* real_buffer, 
                                const float* complex_buffer,
                                int real_buffer_len,
                                bool normalize) {
    // Filling in the buffer with magnitude in ascending frequency order
    
    // complex_buffer[0] = DC
    real_buffer[0] = complex_buffer[0];
    
    // complex_buffer[1] = Nyquist
    if (real_buffer_len >= fft_size_ / 2 + 1) {
        real_buffer[fft_size_ / 2] = complex_buffer[1];
    }

    // The rest of the complex_buffer is interleaved real and imaginary parts
    // For loop starts with i = 1, i.e. complex_buffer[2] 
    for (int i = 1; i < real_buffer_len; ++i) {
        // Real and imaginary parts are interleaved in the output array
        float real = complex_buffer[2 * i];
        float imag = complex_buffer[2 * i + 1];
        float magnitude = sqrtf(real * real + imag * imag);

        // Filling in the buffer
        // Normalized to 1/2N to get amplitude (1/2N because of one-sided FFT)
        real_buffer[i] = normalize ? magnitude / (2 * fft_size_) : magnitude;
    }
}

void FFTProcessor::complexToRealDB(float* real_buffer, 
                                  const float* complex_buffer,
                                  int real_buffer_len,
                                  bool scale,
                                  float floor_db) {
    float floor_db_neg = -std::fabs(floor_db);
    const float epsilon = 1e-20f;

	// Amplitude in dB
	// dynamic when sim, simple when usrp because i cant fucking fix it
	const float fft_len_float = static_cast<float>(fft_size_);
	//const float fft_len_log10 = 20.0f * log10f(fft_len_float);
	const float fft_len_log10 = 20.0f;

    // DC component
    float magnitude_squared = complex_buffer[0] * complex_buffer[0];
    magnitude_squared = fmaxf(magnitude_squared, epsilon);
    float dB = 10.0f * log10f(magnitude_squared) - fft_len_log10;
    dB = fmaxf(dB, floor_db_neg);
    real_buffer[0] = scale ? 1.0f - dB / floor_db_neg : dB;

    // Nyquist component
    if (real_buffer_len >= fft_size_ / 2 + 1) {
        magnitude_squared = complex_buffer[1] * complex_buffer[1];
        magnitude_squared = fmaxf(magnitude_squared, epsilon);
        dB = 10.0f * log10f(magnitude_squared) - fft_len_log10;
        dB = fmaxf(dB, floor_db_neg);
        real_buffer[fft_size_ / 2] = scale ? 1.0f - dB / floor_db_neg : dB;
    }

    // Process the rest of the frequencies
    for (int i = 1; i < real_buffer_len; ++i) {
        float real = complex_buffer[2 * i];
        float imag = complex_buffer[2 * i + 1];
        magnitude_squared = real * real + imag * imag;
        magnitude_squared = fmaxf(magnitude_squared, epsilon);
        dB = 10.0f * log10f(magnitude_squared) - fft_len_log10;
        dB = fmaxf(dB, floor_db_neg);
        real_buffer[i] = scale ? 1.0f - dB / floor_db_neg : dB;
    }
}

void FFTProcessor::complexToPSD(float* psd_buffer,
                               const float* complex_buffer,
                               int psd_buffer_len,
                               float sample_rate,
                               bool db_scale,
                               float floor_db) {

    const float epsilon = 1e-20f; // Small value to avoid log(0)

    // PSD normalization factor: 1 / (Fs * N)
    // For one-sided PSD, we multiply by 2 (except for DC and Nyquist)
    float psd_scale = 1.0f / (sample_rate * fft_size_);

    // DC component (index 0 in complex_buffer)
    float power = complex_buffer[0] * complex_buffer[0];
    float psd = power * psd_scale; // No factor of 2 for DC
    psd = fmaxf(psd, epsilon);

    if (db_scale) {
        float psd_db = 10.0f * log10f(psd);
        psd_db = fmaxf(psd_db, -floor_db);
        psd_buffer[0] = psd_db;
    } else {
        psd_buffer[0] = psd;
    }

    // Nyquist component (index 1 in complex_buffer, maps to fft_size_/2)
    if (psd_buffer_len >= fft_size_ / 2 + 1) {
        power = complex_buffer[1] * complex_buffer[1];
        psd = power * psd_scale; // No factor of 2 for Nyquist
        psd = fmaxf(psd, epsilon);

        if (db_scale) {
            float psd_db = 10.0f * log10f(psd);
            psd_db = fmaxf(psd_db, -floor_db);
            psd_buffer[fft_size_ / 2] = psd_db;
        } else {
            psd_buffer[fft_size_ / 2] = psd;
        }
    }

    // Process the rest of the frequencies (positive frequencies only)
    // These get factor of 2 for one-sided PSD
    for (int i = 1; i < fft_size_ / 2 && i < psd_buffer_len; ++i) {
        float real = complex_buffer[2 * i];
        float imag = complex_buffer[2 * i + 1];
        power = real * real + imag * imag;

        // Factor of 2 for one-sided PSD (double the power from negative frequencies)
        psd = 2.0f * power * psd_scale;
        psd = fmaxf(psd, epsilon);

        if (db_scale) {
            float psd_db = 10.0f * log10f(psd);
            psd_db = fmaxf(psd_db, -floor_db);
            psd_buffer[i] = psd_db;
        } else {
            psd_buffer[i] = psd;
        }
    }
}

float FFTProcessor::binWidth(float sample_freq) const {
    return sample_freq / fft_size_;
}

void FFTProcessor::generateFrequencyArray(float* freq_array, 
                                         int sample_freq, 
                                         int freq_len,
                                         double center_freq) const {
    float bin_width = binWidth(sample_freq);
    
    if (center_freq == 0.0) {
        // Traditional baseband display: 0 to Nyquist
        for (int i = 0; i < freq_len; ++i) {
            freq_array[i] = i * bin_width;
        }
    } else {
        // SDR-style display: center_freq to center_freq + Nyquist (positive offsets only)
        for (int i = 0; i < freq_len; ++i) {
            freq_array[i] = center_freq + (i * bin_width);
        }
    }
}

// SpectrogramAnalyzer implementation

SpectrogramAnalyzer::SpectrogramAnalyzer(int fft_size, float sample_rate)
    : sample_rate_(sample_rate)
    , fft_size_(fft_size)
    , write_pos_(0)
    , spectrum_ready_(false)
	, psd_ready_(false) {
    
    fft_processor_ = std::make_unique<FFTProcessor>(fft_size);
    
    // Allocate buffers
    input_buffer_.resize(fft_size * 2);
    fft_output_.resize(fft_size);
    magnitude_buffer_.resize(fft_processor_->getNumBins());
	psd_buffer_.resize(fft_processor_->getNumBins());
    
    std::cout << "SpectrogramAnalyzer initialized: FFT=" << fft_size 
              << ", bins=" << fft_processor_->getNumBins() << std::endl;
}

void SpectrogramAnalyzer::processSamples(const float* samples, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        input_buffer_[write_pos_] = samples[i];
        write_pos_ = (write_pos_ + 1) % input_buffer_.size();
        
        // Process frame when we have enough samples
        if (write_pos_ % fft_size_ == 0) {
            processFrame();
        }
    }
}

void SpectrogramAnalyzer::processSamples(const std::complex<float>* samples, size_t count) {
    // Extract real part from complex samples for now
    // TODO: Could be enhanced to use complex FFT
    for (size_t i = 0; i < count; ++i) {
        input_buffer_[write_pos_] = samples[i].real();
        write_pos_ = (write_pos_ + 1) % input_buffer_.size();
        
        // Process frame when we have enough samples
        if (write_pos_ % fft_size_ == 0) {
            processFrame();
        }
    }
}

void SpectrogramAnalyzer::processFrame() {
    // Extract latest frame from circular buffer
    std::vector<float> frame(fft_size_);
    size_t read_pos = (write_pos_ + input_buffer_.size() - fft_size_) % input_buffer_.size();
    
    for (int i = 0; i < fft_size_; ++i) {
        frame[i] = input_buffer_[read_pos];
        read_pos = (read_pos + 1) % input_buffer_.size();
    }
    
    // Perform FFT
    fft_processor_->forwardFFT(frame.data(), fft_output_.data());
    
    // Convert to dB magnitude
    fft_processor_->complexToRealDB(magnitude_buffer_.data(), 
                                   fft_output_.data(),
                                   magnitude_buffer_.size(),
                                   true,	// scale
                                   80.0f);

	fft_processor_->complexToPSD(psd_buffer_.data(),
                                fft_output_.data(),
                                psd_buffer_.size(),
                                sample_rate_,
                                false,		// scale
                                80.0f);
    
    spectrum_ready_ = true;
	psd_ready_ = true;
}

bool SpectrogramAnalyzer::getLatestSpectrum(float* output, int output_len) {
    if (!spectrum_ready_) {
        return false;
    }
    
    int copy_len = std::min(output_len, static_cast<int>(magnitude_buffer_.size()));
    std::memcpy(output, magnitude_buffer_.data(), copy_len * sizeof(float));
    
    spectrum_ready_ = false;  // Mark as consumed
    return true;
}

bool SpectrogramAnalyzer::getLatestPSD(float* output, int output_len, bool db_scale) {
    if (!psd_ready_) {
        return false;
    }
    if (db_scale) {
        int copy_len = std::min(output_len, static_cast<int>(psd_buffer_.size()));
        std::memcpy(output, psd_buffer_.data(), copy_len * sizeof(float));
    } else {
        // Convert from dB back to linear if needed
        // For now, just copy dB values - linear conversion can be added later if needed
        int copy_len = std::min(output_len, static_cast<int>(psd_buffer_.size()));
        std::memcpy(output, psd_buffer_.data(), copy_len * sizeof(float));
    }

    psd_ready_ = false;  // Mark as consumed
    return true;
}

void SpectrogramAnalyzer::getFrequencyArray(float* freq_array, int freq_len, double center_freq) {
    fft_processor_->generateFrequencyArray(freq_array, static_cast<int>(sample_rate_), freq_len, center_freq);
}
