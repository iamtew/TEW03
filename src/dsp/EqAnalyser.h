#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <cmath>

namespace tew
{

// Lock-free output spectrum for the EQ page. FFT only when a 2048 hop fills.
struct EqAnalyser
{
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int bins = fftSize / 2 + 1;

    EqAnalyser()
        : fft (fftOrder),
          window ((size_t) fftSize, juce::dsp::WindowingFunction<float>::hann)
    {
    }

    void prepare (double sampleRate)
    {
        sr = sampleRate > 1.0 ? sampleRate : 44100.0;
        fifoIdx = 0;
        for (int s = 0; s < 2; ++s)
            for (int i = 0; i < bins; ++i)
                magDb[s][i] = -90.f;
    }

    void push (const juce::AudioBuffer<float>& buffer)
    {
        const int n = buffer.getNumSamples();
        if (n <= 0 || buffer.getNumChannels() <= 0)
            return;

        const float* l = buffer.getReadPointer (0);
        const float* r = buffer.getNumChannels() > 1 ? buffer.getReadPointer (1) : l;
        for (int i = 0; i < n; ++i)
        {
            fifo[fifoIdx++] = 0.5f * (l[i] + r[i]);
            if (fifoIdx >= fftSize)
            {
                fifoIdx = 0;
                transform();
            }
        }
    }

    void copyLogDb (float* dest, int n) const
    {
        if (dest == nullptr || n <= 0)
            return;
        const int side = published.load (std::memory_order_acquire);
        const float* m = magDb[side];
        const float ny = (float) sr * 0.5f;
        for (int i = 0; i < n; ++i)
        {
            const float x01 = n > 1 ? (float) i / (float) (n - 1) : 0.f;
            const float hz = 20.f * std::pow (10.f, x01 * 3.f);
            const float bin = hz * (float) fftSize / (float) sr;
            const int b0 = juce::jlimit (0, bins - 2, (int) bin);
            const float t = bin - (float) b0;
            const float db = m[b0] + t * (m[b0 + 1] - m[b0]);
            dest[i] = hz >= ny ? -90.f : db;
        }
    }

private:
    void transform()
    {
        for (int i = 0; i < fftSize; ++i)
            fftBuf[i] = fifo[i];
        for (int i = fftSize; i < fftSize * 2; ++i)
            fftBuf[i] = 0.f;

        window.multiplyWithWindowingTable (fftBuf, (size_t) fftSize);
        fft.performFrequencyOnlyForwardTransform (fftBuf, true);

        const int w = 1 - published.load (std::memory_order_relaxed);
        const float norm = 2.f / (float) fftSize;
        for (int i = 0; i < bins; ++i)
        {
            const float mag = fftBuf[i] * norm;
            const float db = mag > 1.0e-8f ? 20.f * std::log10 (mag) : -90.f;
            float& slot = magDb[w][i];
            slot = db > slot ? db : slot * 0.88f + db * 0.12f;
        }
        published.store (w, std::memory_order_release);
    }

    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    float fifo[fftSize] {};
    float fftBuf[fftSize * 2] {};
    float magDb[2][bins] {};
    std::atomic<int> published { 0 };
    int fifoIdx = 0;
    double sr = 44100.0;
};

// Peak / clip tap after FX. Spectrum lives on EqAnalyser.
struct OutputMeter
{
    void prepare (double) {}

    void push (const juce::AudioBuffer<float>& buffer)
    {
        const int n = buffer.getNumSamples();
        if (n <= 0 || buffer.getNumChannels() <= 0)
            return;

        const float* l = buffer.getReadPointer (0);
        const float* r = buffer.getNumChannels() > 1 ? buffer.getReadPointer (1) : l;
        float p = 0.f;
        bool clip = false;
        for (int i = 0; i < n; ++i)
        {
            const float a = std::abs (0.5f * (l[i] + r[i]));
            if (a > p)
                p = a;
            if (a >= 1.f)
                clip = true;
        }
        peak.store (p, std::memory_order_relaxed);
        if (clip)
            clipArmed.store (true, std::memory_order_relaxed);
    }

    float lastPeak() const { return peak.load (std::memory_order_relaxed); }

    bool takeClip()
    {
        return clipArmed.exchange (false, std::memory_order_relaxed);
    }

private:
    std::atomic<float> peak { 0.f };
    std::atomic<bool> clipArmed { false };
};

} // namespace tew
