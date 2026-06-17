// ECGFilterDLL.cpp — ECG signal filtering algorithm implementations
//
// Key MFC DLL concepts demonstrated:
//   - __declspec(dllexport) for C interface export
//   - Main program uses LoadLibrary / GetProcAddress for dynamic loading
//   - Modular design: filtering algorithms separated from UI

#include "stdafx.h"
#include "ECGFilterDLL.h"
#include <cmath>
#include <cstring>
#include <algorithm>

// ============================================================================
// Internal helper functions
// ============================================================================

// Compute median of array (for adaptive threshold)
static double ComputeMedian(const double* arr, int n)
{
    if (n <= 0) return 0.0;
    std::vector<double> copy(arr, arr + n);
    std::sort(copy.begin(), copy.end());
    if (n % 2 == 1)
        return copy[n / 2];
    else
        return (copy[n / 2 - 1] + copy[n / 2]) * 0.5;
}

// Compute maximum value of array
static double ComputeMax(const double* arr, int n)
{
    if (n <= 0) return 0.0;
    double m = arr[0];
    for (int i = 1; i < n; ++i)
        if (arr[i] > m) m = arr[i];
    return m;
}

// ============================================================================
// Exported function implementations
// ============================================================================

ECGFILTERDLL_API int MovingAverageFilter(
    const double* input,
    double* output,
    int n,
    int windowSize)
{
    if (!input || !output || n <= 0 || windowSize <= 0 || windowSize > n)
        return -1;

    int half = windowSize / 2;

    for (int i = 0; i < n; ++i)
    {
        double sum = 0.0;
        int count = 0;

        // Asymmetric window at boundaries
        int start = (std::max)(i - half, 0);
        int end = (std::min)(i + half + (windowSize % 2), n);

        for (int j = start; j < end; ++j)
        {
            sum += input[j];
            ++count;
        }
        output[i] = sum / count;
    }

    return 0;
}

ECGFILTERDLL_API int BandpassFilter(
    const double* input,
    double* output,
    int n,
    int samplingRate)
{
    if (!input || !output || n <= 0 || samplingRate <= 0)
        return -1;

    // Step 1: High-pass differencing (remove baseline drift, < 0.5 Hz)
    // y[i] = x[i] - x[i-k], where k = fs/30
    int kHP = samplingRate / 30;
    if (kHP < 1) kHP = 1;

    std::vector<double> hpResult(n, 0.0);
    for (int i = 0; i < n; ++i)
    {
        if (i >= kHP)
            hpResult[i] = input[i] - input[i - kHP];
        else
            hpResult[i] = input[i] - input[0];  // boundary handling
    }

    // Step 2: Moving average low-pass (remove high frequency noise, > 40 Hz)
    int lpTaps = samplingRate / 40;
    if (lpTaps < 3) lpTaps = 3;
    if (lpTaps % 2 == 0) lpTaps += 1;  // keep odd

    int ret = MovingAverageFilter(hpResult.data(), output, n, lpTaps);

    return ret;
}

ECGFILTERDLL_API int DetectRPeaks(
    const double* signal,
    int n,
    int* peaks,
    int* peakCount,
    int samplingRate)
{
    if (!signal || !peaks || !peakCount || n <= 0 || samplingRate <= 0)
        return -1;

    *peakCount = 0;

    // 1. Compute adaptive threshold
    double maxVal = ComputeMax(signal, n);
    double threshold = maxVal * 0.55;  // R wave typically > 55% of max

    // 2. Refractory period (prevent multiple detections per beat)
    // Normal HR <= 300 BPM => RR >= 0.2 s
    int refractory = (int)(0.2 * samplingRate);
    if (refractory < 30) refractory = 30;

    int lastPeakIdx = -refractory;

    // 3. Sliding window detection
    int searchWindow = samplingRate / 20;  // 50ms search window
    if (searchWindow < 3) searchWindow = 3;

    for (int i = 1; i < n - 1; ++i)
    {
        // Must be outside refractory period
        if (i - lastPeakIdx < refractory)
            continue;

        // Signal must exceed threshold
        if (signal[i] <= threshold)
            continue;

        // Check if local maximum within search window
        bool isLocalMax = true;
        int left = (std::max)(i - searchWindow, 0);
        int right = (std::min)(i + searchWindow, n - 1);

        for (int j = left; j <= right; ++j)
        {
            if (j != i && signal[j] >= signal[i])
            {
                isLocalMax = false;
                break;
            }
        }

        if (isLocalMax)
        {
            peaks[*peakCount] = i;
            (*peakCount)++;
            lastPeakIdx = i;

            if (*peakCount >= n) break;
        }
    }

    return 0;
}

ECGFILTERDLL_API int HeartRateFromRR(
    const int* rrIntervals,
    int n,
    double* heartRates,
    int samplingRate)
{
    if (!rrIntervals || !heartRates || n <= 0 || samplingRate <= 0)
        return -1;

    for (int i = 0; i < n; ++i)
    {
        if (rrIntervals[i] > 0)
            heartRates[i] = 60.0 * samplingRate / rrIntervals[i];
        else
            heartRates[i] = 0.0;
    }

    return 0;
}

ECGFILTERDLL_API const char* GetFilterVersion(void)
{
    return "ECGFilterDLL v1.0 - ECG Signal Filtering & R-Peak Detection Library";
}
