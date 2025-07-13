#include "FFTProcessor.h"
#include <algorithm>
#include <cstring>

constexpr float PI = 3.14159265358979323846f;

FFTProcessor::FFTProcessor(size_t fft_size) 
    : fft_size_(fft_size) {
    work_buffer_.resize(fft_size_);
    // Don't need twiddle factors for Cooley-Tukey FFT
}

FFTProcessor::~FFTProcessor() = default;

// Cooley-Tukey radix-2 FFT implementation
void FFTProcessor::fftRadix2(std::complex<float>* data, size_t n, bool inverse) {
    // Bit reversal
    size_t j = 0;
    for (size_t i = 1; i < n - 1; ++i) {
        size_t bit = n >> 1;
        while (j & bit) {
            j ^= bit;
            bit >>= 1;
        }
        j ^= bit;
        
        if (i < j) {
            std::swap(data[i], data[j]);
        }
    }
    
    // FFT computation
    float direction = inverse ? 1.0f : -1.0f;
    
    for (size_t len = 2; len <= n; len <<= 1) {
        float angle = direction * 2.0f * PI / len;
        std::complex<float> wlen(std::cos(angle), std::sin(angle));
        
        for (size_t i = 0; i < n; i += len) {
            std::complex<float> w(1.0f, 0.0f);
            
            for (size_t j = 0; j < len / 2; ++j) {
                std::complex<float> u = data[i + j];
                std::complex<float> v = data[i + j + len / 2] * w;
                
                data[i + j] = u + v;
                data[i + j + len / 2] = u - v;
                
                w *= wlen;
            }
        }
    }
    
    // Normalize for inverse transform
    if (inverse) {
        float norm = 1.0f / n;
        for (size_t i = 0; i < n; ++i) {
            data[i] *= norm;
        }
    }
}

void FFTProcessor::processReal(const float* input, std::complex<float>* output) {
    // Convert real to complex
    for (size_t i = 0; i < fft_size_; ++i) {
        work_buffer_[i] = std::complex<float>(input[i], 0.0f);
    }
    
    // Copy to output buffer
    std::memcpy(output, work_buffer_.data(), fft_size_ * sizeof(std::complex<float>));
    
    // Perform FFT in-place
    fftRadix2(output, fft_size_, false);
}

void FFTProcessor::processComplex(const std::complex<float>* input, 
                                  std::complex<float>* output) {
    // Copy to output buffer
    std::memcpy(output, input, fft_size_ * sizeof(std::complex<float>));
    
    // Perform FFT in-place
    fftRadix2(output, fft_size_, false);
}

void FFTProcessor::initializeTwiddleFactors() {
    // Not needed for Cooley-Tukey implementation
}

void FFTProcessor::computeDFT(const std::complex<float>* input, 
                              std::complex<float>* output) {
    // Not used - replaced by fftRadix2
}

// Rest of the implementation remains the same...
void FFTProcessor::computeMagnitudeDB(const std::complex<float>* fft_output,
                                      float* magnitude_db,
                                      size_t size,
                                      bool normalize) {
    const float min_db = -120.0f;  // Noise floor
    float max_mag = 0.0f;
    
    // First pass: compute magnitudes and find max if normalizing
    for (size_t i = 0; i < size; ++i) {
        float mag = std::abs(fft_output[i]);
        
        if (normalize && mag > max_mag) {
            max_mag = mag;
        }
        
        // Convert to dB with small epsilon to avoid log(0)
        if (mag > 1e-10f) {
            magnitude_db[i] = 20.0f * std::log10(mag);
        } else {
            magnitude_db[i] = min_db;
        }
    }
    
    // Second pass: normalize if requested
    if (normalize && max_mag > 0.0f) {
        float max_db = 20.0f * std::log10(max_mag);
        for (size_t i = 0; i < size; ++i) {
            magnitude_db[i] -= max_db;
        }
    }
}

void FFTProcessor::computePSD(const std::complex<float>* fft_output,
                             float* psd,
                             size_t size,
                             double sample_rate,
                             size_t fft_size) {
    // PSD = |X(f)|^2 / (fs * N)
    // Where fs is sample rate and N is FFT size
    
    const float scale = 1.0f / (sample_rate * fft_size);
    const float min_psd_db = -160.0f;
    
    for (size_t i = 0; i < size; ++i) {
        float power = std::norm(fft_output[i]) * scale;
        
        // For DC and Nyquist, don't double the power
        if (i != 0 && i != size - 1) {
            power *= 2.0f;  // Account for negative frequencies
        }
        
        if (power > 1e-16f) {
            psd[i] = 10.0f * std::log10(power);
        } else {
            psd[i] = min_psd_db;
        }
    }
}

void FFTProcessor::applyWindow(float* data, size_t size, WindowType window_type) {
    std::vector<float> coeffs(size);
    getWindowCoefficients(coeffs.data(), size, window_type);
    
    for (size_t i = 0; i < size; ++i) {
        data[i] *= coeffs[i];
    }
}

void FFTProcessor::applyWindow(std::complex<float>* data, size_t size, 
                               WindowType window_type) {
    std::vector<float> coeffs(size);
    getWindowCoefficients(coeffs.data(), size, window_type);
    
    for (size_t i = 0; i < size; ++i) {
        data[i] *= coeffs[i];
    }
}

void FFTProcessor::getWindowCoefficients(float* coeffs, size_t size, 
                                        WindowType window_type) {
    switch (window_type) {
        case WindowType::NONE:
            std::fill(coeffs, coeffs + size, 1.0f);
            break;
            
        case WindowType::HAMMING:
            for (size_t i = 0; i < size; ++i) {
                coeffs[i] = 0.54f - 0.46f * std::cos(2.0f * PI * i / (size - 1));
            }
            break;
            
        case WindowType::HANNING:
            for (size_t i = 0; i < size; ++i) {
                coeffs[i] = 0.5f * (1.0f - std::cos(2.0f * PI * i / (size - 1)));
            }
            break;
            
        case WindowType::BLACKMAN:
            for (size_t i = 0; i < size; ++i) {
                float n = i / float(size - 1);
                coeffs[i] = 0.42f - 0.5f * std::cos(2.0f * PI * n) + 
                           0.08f * std::cos(4.0f * PI * n);
            }
            break;
            
        case WindowType::BLACKMAN_HARRIS:
            for (size_t i = 0; i < size; ++i) {
                float n = i / float(size - 1);
                coeffs[i] = 0.35875f - 0.48829f * std::cos(2.0f * PI * n) +
                           0.14128f * std::cos(4.0f * PI * n) - 
                           0.01168f * std::cos(6.0f * PI * n);
            }
            break;
            
        case WindowType::FLAT_TOP:
            for (size_t i = 0; i < size; ++i) {
                float n = i / float(size - 1);
                coeffs[i] = 0.21557895f - 0.41663158f * std::cos(2.0f * PI * n) +
                           0.277263158f * std::cos(4.0f * PI * n) - 
                           0.083578947f * std::cos(6.0f * PI * n) +
                           0.006947368f * std::cos(8.0f * PI * n);
            }
            break;
    }
}

float FFTProcessor::getWindowScalingFactor(WindowType window_type) {
    // Scaling factors for accurate amplitude measurements
    switch (window_type) {
        case WindowType::NONE:           return 1.0f;
        case WindowType::HAMMING:        return 1.85f;
        case WindowType::HANNING:        return 2.0f;
        case WindowType::BLACKMAN:       return 2.80f;
        case WindowType::BLACKMAN_HARRIS: return 2.89f;
        case WindowType::FLAT_TOP:       return 4.18f;
        default:                         return 1.0f;
    }
}

// SpectrumAnalyzer implementation

SpectrumAnalyzer::SpectrumAnalyzer(const Config& config, double sample_rate)
    : config_(config)
    , sample_rate_(sample_rate) {
    
    fft_processor_ = std::make_unique<FFTProcessor>(config.fft_size);
    
    // Calculate overlap
    overlap_samples_ = static_cast<size_t>(config.fft_size * config.overlap_ratio);
    
    // Allocate buffers
    input_buffer_.resize(config.fft_size * 2);  // Double buffer for overlap
    fft_output_.resize(config.fft_size);
    magnitude_accumulator_.resize(config.fft_size / 2 + 1, 0.0f);
    psd_accumulator_.resize(config.fft_size / 2 + 1, 0.0f);
    
    // Pre-calculate window
    window_coeffs_.resize(config.fft_size);
    FFTProcessor::getWindowCoefficients(window_coeffs_.data(), config.fft_size, 
                                       config.window_type);
}

SpectrumAnalyzer::~SpectrumAnalyzer() = default;

void SpectrumAnalyzer::addSamples(const float* samples, size_t count) {
    // Convert to complex and process
    for (size_t i = 0; i < count; ++i) {
        input_buffer_[write_pos_] = std::complex<float>(samples[i], 0.0f);
        write_pos_ = (write_pos_ + 1) % input_buffer_.size();
        samples_available_++;
        
        // Process frame when we have enough samples
        if (samples_available_ >= config_.fft_size) {
            processFrame();
            
            // Handle overlap
            samples_available_ = overlap_samples_;
            write_pos_ = (write_pos_ + input_buffer_.size() - overlap_samples_) 
                       % input_buffer_.size();
        }
    }
}

void SpectrumAnalyzer::addSamples(const std::complex<float>* samples, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        input_buffer_[write_pos_] = samples[i];
        write_pos_ = (write_pos_ + 1) % input_buffer_.size();
        samples_available_++;
        
        if (samples_available_ >= config_.fft_size) {
            processFrame();
            samples_available_ = overlap_samples_;
            write_pos_ = (write_pos_ + input_buffer_.size() - overlap_samples_) 
                       % input_buffer_.size();
        }
    }
}

void SpectrumAnalyzer::processFrame() {
    // Extract frame from circular buffer
    std::vector<std::complex<float>> frame(config_.fft_size);
    size_t read_pos = (write_pos_ + input_buffer_.size() - config_.fft_size) 
                    % input_buffer_.size();
    
    for (size_t i = 0; i < config_.fft_size; ++i) {
        frame[i] = input_buffer_[read_pos];
        read_pos = (read_pos + 1) % input_buffer_.size();
    }
    
    // Remove DC if requested
    if (config_.remove_dc) {
        std::complex<float> dc_sum(0.0f, 0.0f);
        for (const auto& s : frame) {
            dc_sum += s;
        }
        dc_sum /= static_cast<float>(config_.fft_size);
        
        for (auto& s : frame) {
            s -= dc_sum;
        }
    }
    
    // Apply window
    FFTProcessor::applyWindow(frame.data(), config_.fft_size, config_.window_type);
    
    // Compute FFT
    fft_processor_->processComplex(frame.data(), fft_output_.data());
    
    // Accumulate results
    if (averages_accumulated_ == 0) {
        std::fill(magnitude_accumulator_.begin(), magnitude_accumulator_.end(), 0.0f);
        std::fill(psd_accumulator_.begin(), psd_accumulator_.end(), 0.0f);
    }
    
    // Only use positive frequencies
    size_t num_bins = config_.fft_size / 2 + 1;
    
    // Accumulate magnitude
    std::vector<float> mag_db(num_bins);
    FFTProcessor::computeMagnitudeDB(fft_output_.data(), mag_db.data(), num_bins, false);
    
    for (size_t i = 0; i < num_bins; ++i) {
        magnitude_accumulator_[i] += mag_db[i];
    }
    
    // Accumulate PSD
    std::vector<float> psd(num_bins);
    FFTProcessor::computePSD(fft_output_.data(), psd.data(), num_bins, 
                            sample_rate_, config_.fft_size);
    
    for (size_t i = 0; i < num_bins; ++i) {
        psd_accumulator_[i] += psd[i];
    }
    
    averages_accumulated_++;
}

bool SpectrumAnalyzer::isSpectrumReady() const {
    return averages_accumulated_ >= config_.averaging_count;
}

void SpectrumAnalyzer::getFrequencies(float* frequencies, size_t count) const {
    size_t num_bins = std::min(count, config_.fft_size / 2 + 1);
    float bin_width = sample_rate_ / config_.fft_size;
    
    for (size_t i = 0; i < num_bins; ++i) {
        frequencies[i] = i * bin_width;
    }
}

void SpectrumAnalyzer::getMagnitudeSpectrum(float* magnitude_db, size_t count) {
    if (!isSpectrumReady()) return;
    
    size_t num_bins = std::min(count, magnitude_accumulator_.size());
    float scale = 1.0f / averages_accumulated_;
    
    for (size_t i = 0; i < num_bins; ++i) {
        magnitude_db[i] = magnitude_accumulator_[i] * scale;
    }
    
    // Reset for next averaging cycle
    averages_accumulated_ = 0;
}

void SpectrumAnalyzer::getPowerSpectralDensity(float* psd, size_t count) {
    if (!isSpectrumReady()) return;
    
    size_t num_bins = std::min(count, psd_accumulator_.size());
    float scale = 1.0f / averages_accumulated_;
    
    for (size_t i = 0; i < num_bins; ++i) {
        psd[i] = psd_accumulator_[i] * scale;
    }
    
    averages_accumulated_ = 0;
}

void SpectrumAnalyzer::reset() {
    write_pos_ = 0;
    samples_available_ = 0;
    averages_accumulated_ = 0;
    std::fill(input_buffer_.begin(), input_buffer_.end(), std::complex<float>(0.0f, 0.0f));
    std::fill(magnitude_accumulator_.begin(), magnitude_accumulator_.end(), 0.0f);
    std::fill(psd_accumulator_.begin(), psd_accumulator_.end(), 0.0f);
}
