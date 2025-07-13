#pragma once

#include <complex>
#include <vector>
#include <memory>
#include <cmath>

/**
 * FFT Processor for frequency domain analysis
 * This is a simple interface that can be implemented with different FFT libraries
 * (FFTW, Kiss FFT, etc.)
 */
class FFTProcessor {
public:
    enum class WindowType {
        NONE,
        HAMMING,
        HANNING,
        BLACKMAN,
        BLACKMAN_HARRIS,
        FLAT_TOP
    };
    
    FFTProcessor(size_t fft_size);
    ~FFTProcessor();
    
    /**
     * Process real-valued time domain samples
     * @param input Real-valued time domain samples
     * @param output Complex frequency domain output (size = fft_size/2 + 1)
     */
    void processReal(const float* input, std::complex<float>* output);
    
    /**
     * Process complex time domain samples
     * @param input Complex time domain samples
     * @param output Complex frequency domain output
     */
    void processComplex(const std::complex<float>* input, std::complex<float>* output);
    
    /**
     * Compute magnitude spectrum in dB
     * @param fft_output Complex FFT output
     * @param magnitude_db Magnitude in dB (20*log10)
     * @param size Number of frequency bins
     * @param normalize If true, normalize to 0 dB max
     */
    static void computeMagnitudeDB(const std::complex<float>* fft_output, 
                                   float* magnitude_db, 
                                   size_t size,
                                   bool normalize = false);
    
    /**
     * Compute power spectral density
     * @param fft_output Complex FFT output
     * @param psd Power spectral density output
     * @param size Number of frequency bins
     * @param sample_rate Sample rate for proper scaling
     * @param fft_size FFT size for proper scaling
     */
    static void computePSD(const std::complex<float>* fft_output,
                          float* psd,
                          size_t size,
                          double sample_rate,
                          size_t fft_size);
    
    /**
     * Apply window function to time domain data
     * @param data Input/output data
     * @param size Data size
     * @param window_type Type of window to apply
     */
    static void applyWindow(float* data, size_t size, WindowType window_type);
    static void applyWindow(std::complex<float>* data, size_t size, WindowType window_type);
    
    /**
     * Get window coefficients
     * @param coeffs Output array for coefficients
     * @param size Window size
     * @param window_type Type of window
     */
    static void getWindowCoefficients(float* coeffs, size_t size, WindowType window_type);
    
    /**
     * Compute window scaling factor for accurate measurements
     */
    static float getWindowScalingFactor(WindowType window_type);
    
    size_t getFFTSize() const { return fft_size_; }
    
private:
    size_t fft_size_;
    
    // Implementation-specific data (FFTW, Kiss FFT, etc.)
    // For now, using a simple DFT implementation
    std::vector<std::complex<float>> twiddle_factors_;
    std::vector<std::complex<float>> work_buffer_;
    
    void initializeTwiddleFactors();
    void computeDFT(const std::complex<float>* input, std::complex<float>* output);
	void fftRadix2(std::complex<float>* data, size_t n, bool inverse);
};

/**
 * Spectrum analyzer for continuous frequency analysis
 */
class SpectrumAnalyzer {
public:
    struct Config {
        size_t fft_size = 1024;
        FFTProcessor::WindowType window_type = FFTProcessor::WindowType::HAMMING;
        size_t averaging_count = 4;  // Number of FFTs to average
        float overlap_ratio = 0.5f;  // Overlap between FFT frames (0-1)
        bool remove_dc = true;       // Remove DC component
    };
    
    SpectrumAnalyzer(const Config& config, double sample_rate);
    ~SpectrumAnalyzer();
    
    /**
     * Add samples for processing
     * @param samples Input samples (real or complex)
     * @param count Number of samples
     */
    void addSamples(const float* samples, size_t count);
    void addSamples(const std::complex<float>* samples, size_t count);
    
    /**
     * Check if spectrum data is ready
     */
    bool isSpectrumReady() const;
    
    /**
     * Get frequency bins
     * @param frequencies Output array for frequency values
     * @param count Number of bins (should be <= fft_size/2+1)
     */
    void getFrequencies(float* frequencies, size_t count) const;
    
    /**
     * Get magnitude spectrum
     * @param magnitude_db Output array for magnitude in dB
     * @param count Number of bins
     */
    void getMagnitudeSpectrum(float* magnitude_db, size_t count);
    
    /**
     * Get power spectral density
     * @param psd Output array for PSD
     * @param count Number of bins
     */
    void getPowerSpectralDensity(float* psd, size_t count);
    
    /**
     * Reset analyzer state
     */
    void reset();
    
private:
    Config config_;
    double sample_rate_;
    size_t overlap_samples_;
    
    std::unique_ptr<FFTProcessor> fft_processor_;
    
    // Circular buffer for input samples
    std::vector<std::complex<float>> input_buffer_;
    size_t write_pos_ = 0;
    size_t samples_available_ = 0;
    
    // FFT output and averaging
    std::vector<std::complex<float>> fft_output_;
    std::vector<float> magnitude_accumulator_;
    std::vector<float> psd_accumulator_;
    size_t averages_accumulated_ = 0;
    
    // Window coefficients
    std::vector<float> window_coeffs_;
    
    void processFrame();
};
