#pragma once

#include <vector>
#include <complex>
#include <memory>

// Forward declare PFFFT types
typedef struct PFFFT_Setup PFFFT_Setup;

/**
 * STFT-based spectrogram processor that matches the Python implementation
 * Uses overlapping windows and traditional spectrogram computation
 */
class STFTSpectrogram {
public:
    /**
     * Constructor
     * @param fft_size FFT size in samples
     * @param fft_stride Stride between successive FFTs (controls overlap)
     * @param sample_rate Sample rate for frequency axis generation
     */
    STFTSpectrogram(int fft_size, int fft_stride, float sample_rate);
    ~STFTSpectrogram();
    
    // Delete copy constructor and assignment operator
    STFTSpectrogram(const STFTSpectrogram&) = delete;
    STFTSpectrogram& operator=(const STFTSpectrogram&) = delete;
    
    /**
     * Compute 2D spectrogram from IQ samples
     * @param iq_samples Input complex samples
     * @param num_samples Number of input samples
     * @param output_spectrogram Output 2D array (freq_bins x time_frames)
     * @param output_freq_bins Number of frequency bins (output)
     * @param output_time_frames Number of time frames (output)
     * @return true if successful
     */
    bool computeSpectrogram(const std::complex<float>* iq_samples, 
                           size_t num_samples,
                           float** output_spectrogram,
                           int* output_freq_bins,
                           int* output_time_frames);
    
    /**
     * Generate frequency array for the spectrogram
     * @param freq_array Output frequency array
     * @param center_freq Center frequency for SDR-style display (optional)
     */
    void generateFrequencyArray(float* freq_array, double center_freq = 0.0) const;
    
    /**
     * Generate time array for the spectrogram
     * @param time_array Output time array
     * @param num_frames Number of time frames
     */
    void generateTimeArray(float* time_array, int num_frames) const;
    
    // Getters
    int getFFTSize() const { return fft_size_; }
    int getFFTStride() const { return fft_stride_; }
    int getFreqBins() const { return fft_size_; } // Two-sided FFT
    float getSampleRate() const { return sample_rate_; }
    
private:
    int fft_size_;
    int fft_stride_;
    float sample_rate_;
    
    // PFFFT setup for complex-to-complex transforms
    PFFFT_Setup* setup_;
    std::vector<float> work_buffer_;      // Required by PFFFT, pre-allocated once
    std::vector<float> window_function_;  // Blackman window coefficients, computed once
    
    // Helper functions
    bool initialize();
    void cleanup();
    void generateBlackmanWindow();
    void applyWindow(const std::complex<float>* input, float* windowed_output, int offset, size_t input_size);
    void fftShift(float* data, int size);
    void reverseFrequencyBins(float* spectrogram_data, int freq_bins, int time_frames);
    int calculateNumFrames(size_t num_samples) const;
    void convertToDecibels(float* spectrogram_data, int total_elements);
};
