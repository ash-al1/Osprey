#include "STFTSpectrogram.h"
#include <pffft.h>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <cstring>

STFTSpectrogram::STFTSpectrogram(int fft_size, int fft_stride, float sample_rate)
    : fft_size_(fft_size)
    , fft_stride_(fft_stride)
    , sample_rate_(sample_rate)
    , setup_(nullptr) {
    
    // Validate parameters
    if (fft_stride <= 0 || fft_stride > fft_size) {
        throw std::invalid_argument("FFT stride must be: 0 < fft_stride <= fft_size");
    }
    
    if (!initialize()) {
        throw std::runtime_error("Failed to initialize STFTSpectrogram");
    }
    
    std::cout << "STFTSpectrogram initialized: FFT=" << fft_size 
              << ", stride=" << fft_stride << ", overlap=" 
              << (100.0f * (fft_size - fft_stride) / fft_size) << "%" << std::endl;
}

STFTSpectrogram::~STFTSpectrogram() {
    cleanup();
}

bool STFTSpectrogram::initialize() {
    try {
        // Initialize PFFFT for complex-to-complex transforms
        setup_ = pffft_new_setup(fft_size_, PFFFT_COMPLEX);
        if (!setup_) {
            std::cerr << "Failed to create PFFFT setup for complex FFT, size=" << fft_size_ << std::endl;
            return false;
        }
        work_buffer_.resize(fft_size_ * 2);
        generateBlackmanWindow();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Exception during STFTSpectrogram initialization: " << e.what() << std::endl;
        return false;
    }
}

void STFTSpectrogram::cleanup() {
    if (setup_) {
        pffft_destroy_setup(setup_);
        setup_ = nullptr;
    }
}

void STFTSpectrogram::generateBlackmanWindow() {
    window_function_.resize(fft_size_);
    
    // Blackman window: w(n) = 0.42 - 0.5*cos(2πn/N) + 0.08*cos(4πn/N)
    const float a0 = 0.42f;
    const float a1 = 0.5f;
    const float a2 = 0.08f;
    
    for (int n = 0; n < fft_size_; ++n) {
        float w = a0 
                - a1 * std::cos(2.0f * M_PI * n / (fft_size_ - 1))
                + a2 * std::cos(4.0f * M_PI * n / (fft_size_ - 1));
        window_function_[n] = w;
    }
}

int STFTSpectrogram::calculateNumFrames(size_t num_samples) const {
    if (num_samples < static_cast<size_t>(fft_size_)) {
        return 1;
    }
    return static_cast<int>((num_samples - fft_size_) / fft_stride_) + 1;
}

bool STFTSpectrogram::computeSpectrogram(const std::complex<float>* iq_samples, 
		size_t num_samples, float** output_spectrogram,
		int* output_freq_bins, int* output_time_frames) {
    
    if (!setup_ || !iq_samples || !output_spectrogram || !output_freq_bins || !output_time_frames) {
        std::cerr << "Invalid parameters for computeSpectrogram" << std::endl;
        return false;
    }
    
    // Calculate dimensions
    int num_frames = calculateNumFrames(num_samples);
    int freq_bins = fft_size_; // Two-sided FFT
    
    *output_freq_bins = freq_bins;
    *output_time_frames = num_frames;
    
    std::vector<float> fft_input(fft_size_ * 2);
    std::vector<float> fft_output(fft_size_ * 2);
    
    // Process each frame
    for (int frame = 0; frame < num_frames; ++frame) {
        int sample_offset = frame * fft_stride_;
        
        // Apply windowing to current frame
        applyWindow(iq_samples, fft_input.data(), sample_offset, num_samples);
        
        // Perform FFT
        pffft_transform_ordered(setup_, 
                               fft_input.data(), 
                               fft_output.data(), 
                               work_buffer_.data(), 
                               PFFFT_FORWARD);
        
        // Compute power spectrum and apply FFT shift
        std::vector<float> power_spectrum(freq_bins);
        for (int k = 0; k < freq_bins; ++k) {
            float real = fft_output[2*k];
            float imag = fft_output[2*k + 1];
            power_spectrum[k] = real * real + imag * imag;
        }
        
        // Apply FFT shift to center DC component
        fftShift(power_spectrum.data(), freq_bins);
        
        // Store in output array (freq_bins x time_frames layout)
        for (int k = 0; k < freq_bins; ++k) {
            (*output_spectrogram)[k * num_frames + frame] = power_spectrum[k];
        }
    }
    
    // Convert to decibels and reverse frequency bins
    convertToDecibels(*output_spectrogram, freq_bins * num_frames);
    reverseFrequencyBins(*output_spectrogram, freq_bins, num_frames);
    
    return true;
}

void STFTSpectrogram::applyWindow(const std::complex<float>* input, 
		float* windowed_output, int offset, size_t input_size) {
    
    for (int n = 0; n < fft_size_; ++n) {
        int sample_idx = offset + n;
        
        if (sample_idx >= 0 && sample_idx < static_cast<int>(input_size)) {
            // Apply window function
            float window_val = window_function_[n];
            windowed_output[2*n]     = input[sample_idx].real() * window_val; // Real part
            windowed_output[2*n + 1] = input[sample_idx].imag() * window_val; // Imag part
        } else {
            // Zero padding
            windowed_output[2*n]     = 0.0f;
            windowed_output[2*n + 1] = 0.0f;
        }
    }
}

void STFTSpectrogram::fftShift(float* data, int size) {
    int half_size = size / 2;
    
    for (int i = 0; i < half_size; ++i) {
        std::swap(data[i], data[i + half_size]);
    }
    
    if (size % 2 == 1) {
        float temp = data[size - 1];
        for (int i = size - 1; i > half_size; --i) {
            data[i] = data[i - 1];
        }
        data[half_size] = temp;
    }
}

void STFTSpectrogram::reverseFrequencyBins(float* spectrogram_data, 
		int freq_bins, int time_frames) {
    
    for (int frame = 0; frame < time_frames; ++frame) {
        for (int k = 0; k < freq_bins / 2; ++k) {
            int top_idx = k * time_frames + frame;
            int bottom_idx = (freq_bins - 1 - k) * time_frames + frame;
            std::swap(spectrogram_data[top_idx], spectrogram_data[bottom_idx]);
        }
    }
}

void STFTSpectrogram::convertToDecibels(float* spectrogram_data, int total_elements) {
    float max_val = 0.0f;
    for (int i = 0; i < total_elements; ++i) {
        max_val = std::max(max_val, std::abs(spectrogram_data[i]));
    }
    
    float epsilon = max_val * std::sqrt(1e-20f);
    
    for (int i = 0; i < total_elements; ++i) {
        float val = spectrogram_data[i];
        if (val <= 0.0f) {
            val = epsilon;
        }
        spectrogram_data[i] = 10.0f * std::log10(val);
    }
}

void STFTSpectrogram::generateFrequencyArray(float* freq_array, double center_freq) const {
    float bin_width = sample_rate_ / fft_size_;
    
    for (int k = 0; k < fft_size_; ++k) {
        int shifted_k = (k + fft_size_/2) % fft_size_ - fft_size_/2;
        freq_array[k] = center_freq + shifted_k * bin_width;
    }
}

void STFTSpectrogram::generateTimeArray(float* time_array, int num_frames) const {
    float time_step = static_cast<float>(fft_stride_) / sample_rate_;
    
    for (int frame = 0; frame < num_frames; ++frame) {
        time_array[frame] = frame * time_step;
    }
}
