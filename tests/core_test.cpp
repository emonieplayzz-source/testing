#include "../Source/AudioCore.h"
#include <cstdio>
#include <random>
#include <thread>
using namespace lumivore;
static int fails = 0;
#define CHECK(c, msg) do { if (!(c)) { std::printf("FAIL: %s\n", msg); ++fails; } else std::printf("ok:   %s\n", msg); } while (0)
using Q = SpscFifo<float, 1 << 16>;
static Q q;
static void feed (Analyzer& a, const std::vector<float>& x) { for (float v : x) q.push (v); a.pump (q); }
static void feedAll (Analyzer& a, const std::vector<float>& x)
{ for (size_t o = 0; o < x.size(); o += 4096) { size_t n = std::min<size_t> (4096, x.size() - o); feed (a, std::vector<float> (x.begin() + (long) o, x.begin() + (long) (o + n))); } }

int main()
{
    const double sr = 48000.0;
    {
        Analyzer a; a.setSampleRate (sr); std::vector<float> x (16384);
        for (size_t i = 0; i < x.size(); ++i) x[i] = 0.5f * (float) std::sin (2 * kPi * 1000.0 * (double) i / sr);
        feedAll (a, x);
        const auto& s = a.features().spectrum;
        int pk = (int) (std::max_element (s.begin(), s.end()) - s.begin());
        CHECK (std::abs (pk - 1000.0 / (sr / Analyzer::kFft)) <= 1.0, "1 kHz sine peaks at expected FFT bin");
        CHECK (std::abs (a.features().rms - 0.5 / std::sqrt (2.0)) < 0.01, "RMS of 0.5-amplitude sine ~= 0.354");
        CHECK (a.features().mid > a.features().bass && a.features().mid > a.features().treble, "1 kHz energy classified as mid band");
    }
    {
        Analyzer a; a.setSampleRate (sr); std::vector<float> x (8192);
        for (size_t i = 0; i < x.size(); ++i) x[i] = 0.5f * (float) std::sin (2 * kPi * 80.0 * (double) i / sr);
        feedAll (a, x);
        CHECK (a.features().bass > a.features().mid && a.features().bass > a.features().treble, "80 Hz energy classified as bass band");
        Analyzer t; t.setSampleRate (sr);
        for (size_t i = 0; i < x.size(); ++i) x[i] = 0.5f * (float) std::sin (2 * kPi * 9000.0 * (double) i / sr);
        feedAll (t, x);
        CHECK (t.features().treble > t.features().bass && t.features().treble > t.features().mid, "9 kHz energy classified as treble band");
    }
    {
        Analyzer a; a.setSampleRate (sr);
        const int total = (int) (sr * 10), period = (int) (sr * 0.5);
        std::vector<float> x ((size_t) total, 0.f);
        for (int k = 0; k < 20; ++k)
            for (int i = 0; i < (int) (0.15 * sr) && k * period + i < total; ++i)
                x[(size_t) (k * period + i)] = 0.9f * (float) (std::sin (2 * kPi * 60.0 * i / sr) * std::exp (-i / (0.04 * sr)));
        feedAll (a, x);
        std::printf ("      kick beats detected: %d (20 kicks played)\n", a.features().beatCount);
        CHECK (a.features().beatCount >= 16 && a.features().beatCount <= 24, "beat detector finds ~20 kicks (16..24)");

        Analyzer b; b.setSampleRate (sr); std::mt19937 g (1); std::normal_distribution<float> d (0.f, 0.1f);
        std::vector<float> nz (4096);
        for (int r = 0; r < 20; ++r) { for (auto& v : nz) v = d (g); feed (b, nz); }   // warm-up (onset from silence is legitimate)
        const int warm = b.features().beatCount;
        for (int r = 0; r < 100; ++r) { for (auto& v : nz) v = d (g); feed (b, nz); }  // ~8.5 s of steady noise
        std::printf ("      steady-noise beats after warm-up: %d (warm-up onsets: %d)\n", b.features().beatCount - warm, warm);
        CHECK (b.features().beatCount - warm == 0, "steady noise does not trigger beats after warm-up");
    }
    {
        static SpscFifo<float, 1 << 12> f2; const int N = 2000000; std::atomic<bool> bad { false };
        std::thread prod ([&] { for (int i = 0; i < N;) if (f2.push ((float) i)) ++i; });
        std::thread cons ([&] { float v, last = -1; int got = 0; while (got < N) if (f2.pop (v)) { if (v != last + 1) bad = true; last = v; ++got; } });
        prod.join(); cons.join();
        CHECK (!bad.load(), "SPSC FIFO preserves order across 2M samples between two threads");
        static SpscFifo<float, 1 << 4> small; int ok = 0; for (int i = 0; i < 100; ++i) ok += small.push (1.f);
        CHECK (ok == 15, "full FIFO rejects pushes instead of blocking");
    }
    {
        Analyzer a; a.setSampleRate (sr); float col[512]; a.logSpectrum (col, 512);
        bool inr = true; for (float v : col) inr = inr && (v >= 0.f && v <= 1.f);
        CHECK (inr, "logSpectrum output within 0..1");
    }
    std::printf (fails ? "\n%d TEST(S) FAILED\n" : "\nALL TESTS PASSED\n", fails);
    return fails ? 1 : 0;
}
