#pragma once

#include <vector>
#include <memory>
#include <complex>

// Forward declare PFFFT types
typedef struct PFFFT_Setup PFFFT_Setup;

/**
 * Simple FFT processor based on Spectrolysis implementation
 * Uses PFFFT for high-performance real-to-complex transforms
 */
class FFTProcessor {
public:
    explicit FFTProcessor(int fft_size);
    ~FFTProcessor();
    
    // Delete copy constructor and assignment operator
    FFTProcessor(const FFTProcessor&) = delete;
    FFTProcessor& operator=(const FFTProcessor&) = delete;
    
    /**
     * Perform forward FFT on real input
     * @param input_buffer Real input samples (size = fft_size)
     * @param output_buffer Complex output (size = fft_size, interleaved real/imag)
     */
    void forwardFFT(const float* input_buffer, float* output_buffer);
    
    /**
     * Convert complex FFT output to real magnitude values
     * @param real_buffer Output magnitude buffer
     * @param complex_buffer Complex FFT output from forwardFFT
     * @param real_buffer_len Length of output buffer (usually fft_size/2 + 1)
     * @param normalize If true, normalize by 1/2N for amplitude
     */
    void complexToReal(float* real_buffer, 
                      const float* complex_buffer,
                      int real_buffer_len,
                      bool normalize = false);
    
    /**
     * Convert complex FFT output to dB magnitude values
     * @param real_buffer Output dB buffer
     * @param complex_buffer Complex FFT output from forwardFFT
     * @param real_buffer_len Length of output buffer (usually fft_size/2 + 1)
     * @param scale If true, scale dB values 0-1 for display
     * @param floor_db Noise floor level in dB (positive value, will be made negative)
     */
    void complexToRealDB(float* real_buffer, 
                        const float* complex_buffer,
                        int real_buffer_len,
                        bool scale = true,
                        float floor_db = 80.0f);
    
    /**
     * Calculate frequency bin width
     * @param sample_freq Sample rate in Hz
     * @return Frequency width per bin in Hz
     */
    float binWidth(float sample_freq) const;
    
    /**
     * Generate frequency array for FFT bins
     * @param freq_array Output frequency array
     * @param sample_freq Sample rate in Hz
     * @param freq_len Length of frequency array
     * @param center_freq Center frequency for SDR-style display (optional)
     */
    void generateFrequencyArray(float* freq_array, 
                               int sample_freq, 
                               int freq_len,
                               double center_freq = 0.0) const;
    
    int getFFTSize() const { return fft_size_; }
    int getNumBins() const { return fft_size_ / 2 + 1; }
    
private:
    PFFFT_Setup* setup_;
    std::vector<float> work_buffer_;
    int fft_size_;
    
    void cleanup();
};

/**
 * Simple spectrogram analyzer that processes audio samples
 * and generates magnitude spectra for display
 */
class SpectrogramAnalyzer {
public:
    SpectrogramAnalyzer(int fft_size, float sample_rate);
    ~SpectrogramAnalyzer() = default;
    
    /**
     * Process audio samples and update spectrogram
     * @param samples Input audio samples
     * @param count Number of samples
     */
    void processSamples(const float* samples, size_t count);
    void processSamples(const std::complex<float>* samples, size_t count);
    
    /**
     * Get the latest magnitude spectrum in dB
     * @param output Output buffer for magnitude spectrum
     * @param output_len Length of output buffer
     * @return true if new data is available
     */
    bool getLatestSpectrum(float* output, int output_len);
    
    /**
     * Get frequency array for the spectrum bins
     * @param freq_array Output frequency array
     * @param freq_len Length of frequency array
     * @param center_freq Center frequency for SDR-style display (optional)
     */
    void getFrequencyArray(float* freq_array, int freq_len, double center_freq = 0.0);
    
    int getNumBins() const { return fft_processor_->getNumBins(); }
    
private:
    std::unique_ptr<FFTProcessor> fft_processor_;
    std::vector<float> input_buffer_;
    std::vector<float> fft_output_;
    std::vector<float> magnitude_buffer_;
    
    float sample_rate_;
    int fft_size_;
    size_t write_pos_;
    bool spectrum_ready_;
    
    void processFrame();
};
