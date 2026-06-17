// ECGFilterDLL.h — DLL export interface
// Exports: MovingAverageFilter, BandpassFilter, DetectRPeaks, HeartRateFromRR
//
// Exported functions:
//   MovingAverageFilter  - Moving average filter (remove high-freq noise)
//   BandpassFilter       - Bandpass filter (0.5-40 Hz, remove baseline drift and EMG noise)
//   DetectRPeaks         - R-peak detection (adaptive threshold method)
//   HeartRateFromRR      - Calculate heart rate from RR intervals

#pragma once

#ifdef ECGFILTERDLL_EXPORTS
#define ECGFILTERDLL_API __declspec(dllexport)
#else
#define ECGFILTERDLL_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Moving average filter
// @param input      Input signal array
// @param output     Output signal array (caller-allocated, at least n elements)
// @param n          Signal length
// @param windowSize Moving window size (recommended: 5-15)
// @return 0 on success, -1 on invalid parameters
ECGFILTERDLL_API int MovingAverageFilter(
    const double* input,
    double* output,
    int n,
    int windowSize
);

// Simple bandpass filter (cascade: high-pass diff + moving average low-pass)
// Removes baseline drift (<0.5Hz) and high-frequency noise (>40Hz)
// @param input        Input signal array
// @param output       Output signal array (caller-allocated, at least n elements)
// @param n            Signal length
// @param samplingRate Sampling rate in Hz (e.g. 360)
// @return 0 on success, -1 on invalid parameters
ECGFILTERDLL_API int BandpassFilter(
    const double* input,
    double* output,
    int n,
    int samplingRate
);

// Detect R-peak positions
// @param signal       Input signal (pre-filtering recommended)
// @param n            Signal length
// @param peaks        Output peak index array (caller-allocated, at least n elements)
// @param peakCount    Output actual number of detected peaks
// @param samplingRate Sampling rate in Hz
// @return 0 on success, -1 on invalid parameters
ECGFILTERDLL_API int DetectRPeaks(
    const double* signal,
    int n,
    int* peaks,
    int* peakCount,
    int samplingRate
);

// Calculate instantaneous heart rate from RR intervals
// @param rrIntervals  RR interval array (unit: samples)
// @param n            Number of RR intervals
// @param heartRates   Output heart rate array (BPM)
// @param samplingRate Sampling rate in Hz
// @return 0 on success
ECGFILTERDLL_API int HeartRateFromRR(
    const int* rrIntervals,
    int n,
    double* heartRates,
    int samplingRate
);

// Get DLL version string
ECGFILTERDLL_API const char* GetFilterVersion(void);

#ifdef __cplusplus
}
#endif
