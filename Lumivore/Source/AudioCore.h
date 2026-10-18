// Lumivore audio core: JUCE-free, allocation-free on the audio side, unit-testable.
#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace lumivore
{
constexpr double kPi = 3.14159265358979323846;

// Single-producer / single-consumer lock-free FIFO. Audio thread pushes, render thread pops.
template <typename T, size_t Capacity>
class SpscFifo
{
    static_assert ((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");
public:
    bool push (T v) noexcept
    {
        const auto h = head.load (std::memory_order_relaxed);
        const auto n = (h + 1) & mask;
        if (n == tail.load (std::memory_order_acquire)) return false; // full: drop, never block
        buf[h] = v;
        head.store (n, std::memory_order_release);
        return true;
    }
    bool pop (T& v) noexcept
    {
        const auto t = tail.load (std::memory_order_relaxed);
        if (t == head.load (std::memory_order_acquire)) return false;
        v = buf[t];
        tail.store ((t + 1) & mask, std::memory_order_release);
        return true;
    }
    void clearFromConsumer() noexcept { T v; while (pop (v)) {} }
private:
    static constexpr size_t mask = Capacity - 1;
    std::array<T, Capacity> buf {};
    alignas (64) std::atomic<size_t> head { 0 };
    alignas (64) std::atomic<size_t> tail { 0 };
};

// Iterative radix-2 complex FFT (size must be a power of two).
class FFT
{
public:
    explicit FFT (int n) : size (n), cosT ((size_t) n / 2), sinT ((size_t) n / 2), rev ((size_t) n)
    {
        int bits = 0; while ((1 << bits) < n) ++bits;
        for (int i = 0; i < n; ++i)
        {
            int r = 0; for (int b = 0; b < bits; ++b) if (i & (1 << b)) r |= 1 << (bits - 1 - b);
            rev[(size_t) i] = r;
        }
        for (int i = 0; i < n / 2; ++i)
        {
            cosT[(size_t) i] = (float) std::cos (2.0 * kPi * i / n);
            sinT[(size_t) i] = (float) -std::sin (2.0 * kPi * i / n);
        }
    }
    void forward (float* re, float* im) const noexcept
    {
        for (int i = 0; i < size; ++i)
        {
            const int j = rev[(size_t) i];
            if (j > i) { std::swap (re[i], re[j]); std::swap (im[i], im[j]); }
        }
        for (int len = 2; len <= size; len <<= 1)
        {
            const int half = len / 2, step = size / len;
            for (int i = 0; i < size; i += len)
                for (int k = 0; k < half; ++k)
                {
                    const float wr = cosT[(size_t) (k * step)], wi = sinT[(size_t) (k * step)];
                    const int a = i + k, b = a + half;
                    const float tr = re[b] * wr - im[b] * wi, ti = re[b] * wi + im[b] * wr;
                    re[b] = re[a] - tr; im[b] = im[a] - ti;
                    re[a] += tr;        im[a] += ti;
                }
        }
    }
private:
    int size;
    std::vector<float> cosT, sinT;
    std::vector<int> rev;
};

struct Features
{
    static constexpr int kBins = 1024, kWave = 512;
    float bass = 0, mid = 0, treble = 0, rms = 0;
    float beat = 0;        // 1.0 on the hop a beat is detected, else 0
    float beatPulse = 0;   // 1.0 on beat, decays smoothly
    int   beatCount = 0;
    std::array<float, kBins> spectrum {};  // smoothed, 0..1
    std::array<float, kWave> wave {};      // last 512 samples, -1..1
};

// Runs entirely on the consumer (render) thread. All buffers are preallocated.
class Analyzer
{
public:
    static constexpr int kFft = 2048, kHop = 512;

    Analyzer() : fft (kFft), window (kFft), ring (kFft, 0.f), re (kFft), im (kFft),
                 prevRaw (Features::kBins, 0.f), fluxHist (kHistory, 0.f)
    {
        for (int i = 0; i < kFft; ++i) window[(size_t) i] = (float) (0.5 - 0.5 * std::cos (2.0 * kPi * i / kFft));
    }

    void setSampleRate (double sr) noexcept { sampleRate = sr > 1000.0 ? sr : 44100.0; }

    template <typename Fifo>
    void pump (Fifo& fifo) noexcept
    {
        float s;
        while (fifo.pop (s))
        {
            ring[(size_t) writePos] = s;
            writePos = (writePos + 1) % kFft;
            if (++sinceHop >= kHop) { sinceHop = 0; analyse(); }
        }
    }

    const Features& features() const noexcept { return f; }

    // Log-spaced resample of the smoothed spectrum (30 Hz..18 kHz) into n columns, 0..1.
    void logSpectrum (float* dst, int n) const noexcept
    {
        const double binHz = sampleRate / kFft;
        for (int c = 0; c < n; ++c)
        {
            const double hz = 30.0 * std::pow (18000.0 / 30.0, (double) c / (n - 1));
            const double pos = std::clamp (hz / binHz, 1.0, (double) Features::kBins - 2);
            const int i = (int) pos; const float fr = (float) (pos - i);
            dst[c] = f.spectrum[(size_t) i] * (1.f - fr) + f.spectrum[(size_t) i + 1] * fr;
        }
    }

private:
    static constexpr int kHistory = 43;
    static constexpr float kFloorDb = -80.f;

    int bin (double hz) const noexcept { return std::clamp ((int) (hz / (sampleRate / kFft)), 1, Features::kBins - 1); }

    float bandAvg (double lo, double hi) const noexcept
    {
        const int a = bin (lo), b = std::max (a + 1, bin (hi));
        float s = 0; for (int i = a; i < b; ++i) s += f.spectrum[(size_t) i];
        return s / (float) (b - a);
    }

    void analyse() noexcept
    {
        double sq = 0;
        for (int i = 0; i < kFft; ++i)
        {
            const float x = ring[(size_t) ((writePos + i) % kFft)];   // oldest -> newest
            re[(size_t) i] = x * window[(size_t) i]; im[(size_t) i] = 0.f; sq += (double) x * x;
            if (i >= kFft - Features::kWave) f.wave[(size_t) (i - (kFft - Features::kWave))] = std::clamp (x, -1.f, 1.f);
        }
        fft.forward (re.data(), im.data());

        // Hann coherent gain 0.5 -> a sine of amplitude A peaks at A * N/4.
        const float norm = 4.f / (float) kFft;
        float flux = 0; const int fluxMax = bin (2000.0);
        for (int i = 0; i < Features::kBins; ++i)
        {
            const float mag = std::sqrt (re[(size_t) i] * re[(size_t) i] + im[(size_t) i] * im[(size_t) i]) * norm;
            const float db = 20.f * std::log10 (std::max (mag, 1e-7f));
            const float n01 = std::clamp ((db - kFloorDb) / -kFloorDb, 0.f, 1.f);
            if (i > 0 && i <= fluxMax) flux += std::max (0.f, n01 - prevRaw[(size_t) i]);
            prevRaw[(size_t) i] = n01;
            float& s = f.spectrum[(size_t) i];
            s += (n01 - s) * (n01 > s ? 0.6f : 0.12f);   // fast attack, slow release
        }
        f.bass   = bandAvg (20.0, 250.0);
        f.mid    = bandAvg (250.0, 4000.0);
        f.treble = bandAvg (4000.0, 16000.0);
        f.rms    = (float) std::sqrt (sq / kFft);

        // Beat: spectral flux above an adaptive threshold, with a ~0.12 s refractory period.
        float mean = 0; for (float v : fluxHist) mean += v; mean /= (float) kHistory;
        float var = 0;  for (float v : fluxHist) var += (v - mean) * (v - mean); var /= (float) kHistory;
        const float threshold = mean + 3.0f * std::sqrt (var) + 0.6f;   // adapts to how "busy" the signal is
        const int refractory = std::max (1, (int) (0.12 * sampleRate / kHop));
        f.beat = 0.f;
        if (cooldown > 0) --cooldown;
        if (cooldown == 0 && flux > threshold && f.rms > 0.01f)
        {
            f.beat = 1.f; f.beatPulse = 1.f; ++f.beatCount; cooldown = refractory;
        }
        f.beatPulse *= 0.90f;
        fluxHist[(size_t) histPos] = flux; histPos = (histPos + 1) % kHistory;
    }

    FFT fft;
    std::vector<float> window, ring, re, im, prevRaw, fluxHist;
    Features f;
    double sampleRate = 44100.0;
    int writePos = 0, sinceHop = 0, histPos = 0, cooldown = 0;
};
} // namespace lumivore
