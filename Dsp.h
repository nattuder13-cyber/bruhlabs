#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>

namespace mk
{
// ---------------------------------------------------------------------------
// Pitch envelope safety limits. Enforced in the parameter ranges AND again in
// the voice DSP (on the final value), so nothing can push below them.
// ---------------------------------------------------------------------------
constexpr float kMinPitchDecayMs    = 15.0f;
constexpr float kMinPitchDepthSemis = 4.0f;   // easy to change
constexpr float kMaxPitchDepthSemis = 60.0f;  // 5 octaves
constexpr float kPi = 3.14159265358979f;

// Snapshot of all parameters, read once per block.
struct Params
{
    float shape = 0.2f, shapeEnv = 0.f;
    float vcaMs = 450.f, vcaCurve = 0.5f, velSens = 0.5f;
    float pMs = 60.f, pCurve = 0.5f, pDepth = 23.f, endHz = 48.f;
    bool  keyTrack = false;
    float f1Ms = 200.f, f1Curve = 0.5f, f1Amt = 0.f;
    bool  f1Link = false;
    int   f1Mode = 0;  float f1Cut = 20000.f, f1Res = 0.f;
    int   d1Type = 0;  float d1Drive = 0.15f, d1Bias = 0.f, d1Mix = 1.f;
    int   f2Mode = 0;  float f2Cut = 20000.f, f2Res = 0.f;
    int   d2Type = 0;  float d2Drive = 0.15f, d2Bias = 0.f, d2Mix = 1.f;
    float outGain = 0.7f;
    // LFO
    float lfoDepth = 0.f;  int lfoWave = 0;  int lfoMode = 0;  bool lfoUni = false;
    float lfoHz = 2.f;
    float lfoPitch = 0.f, lfoShape = 0.f, lfoF1 = 0.f, lfoF2 = 0.f,
          lfoD1 = 0.f, lfoD2 = 0.f, lfoVca = 0.f;
};

// ---------------------------------------------------------------------------
// Lock-free scope capture. Audio thread = single producer, UI = reader.
// ---------------------------------------------------------------------------
struct ScopeBuffer
{
    static constexpr int size = 1 << 18;
    static constexpr uint64_t mask = size - 1;

    std::array<std::atomic<float>, size> data;
    std::atomic<uint64_t> writePos { 0 }, noteOnPos { 0 };
    std::atomic<int> tap { 5 };   // 0 osc,1 F1,2 D1,3 F2,4 D2,5 output

    ScopeBuffer() { for (auto& d : data) d.store (0.f, std::memory_order_relaxed); }

    void push (float x)
    {
        auto w = writePos.load (std::memory_order_relaxed);
        data[(size_t) (w & mask)].store (x, std::memory_order_relaxed);
        writePos.store (w + 1, std::memory_order_release);
    }
    void markNoteOn() { noteOnPos.store (writePos.load (std::memory_order_relaxed), std::memory_order_release); }
};

// ---------------------------------------------------------------------------
// Simple one-shot decay envelope. Phase based: level = f(t/decay), t in [0,1].
// curve k=0 -> linear, larger k -> more exponential. Reaches exactly 0 at t=1,
// so the "decay time" really is the length of the sweep.
// ---------------------------------------------------------------------------
struct DecayEnv
{
    float pos = 1.f;  // >= 1 means finished
    float tick (float decaySamples, float k)
    {
        if (pos >= 1.f) return 0.f;
        float v;
        if (k < 1e-3f) v = 1.f - pos;
        else { const float e = std::exp (-k); v = (std::exp (-k * pos) - e) / (1.f - e); }
        pos += 1.f / std::max (1.f, decaySamples);
        return v;
    }
};

// ---------------------------------------------------------------------------
// Zero-delay-feedback (TPT) state variable filter. mode: 0 LP, 1 BP, 2 HP, 3 Notch
// ---------------------------------------------------------------------------
struct Svf
{
    float s1 = 0.f, s2 = 0.f;
    void reset() { s1 = s2 = 0.f; }
    float process (float x, float cutoff, float res, int mode, float sr)
    {
        cutoff = std::clamp (cutoff, 20.f, sr * 0.45f);
        const float g  = std::tan (kPi * cutoff / sr);
        const float k  = std::max (0.1f, 2.f * (1.f - res));
        const float a1 = 1.f / (1.f + g * (g + k));
        const float a2 = g * a1, a3 = g * a2;
        const float v3 = x - s2;
        const float v1 = a1 * s1 + a2 * v3;
        const float v2 = s2 + a2 * s1 + a3 * v3;
        s1 = 2.f * v1 - s1;
        s2 = 2.f * v2 - s2;
        switch (mode)
        {
            case 0:  return v2;
            case 1:  return v1;
            case 2:  return x - k * v1 - v2;
            default: return x - k * v1;
        }
    }
};

// Saturator. type: 0 tanh, 1 asymmetric, 2 wavefolder.
inline float saturate (float x, int type, float drive, float bias, float mix)
{
    const float g = 1.f + 30.f * drive * drive;
    const float b = bias * 0.5f;
    float y;
    if (type == 0)
        y = (std::tanh (g * x + b) - std::tanh (b)) / std::tanh (g);
    else if (type == 1)
    {
        const float u = g * x + b;
        y = (u > 0.f ? std::tanh (u) : (std::exp (u) - 1.f)) / std::tanh (g);
    }
    else
        y = std::sin ((g * x + b) * kPi * 0.5f);
    return x + mix * (y - x);
}

struct Lfo
{
    float phase = 0.f, sh = 0.f;
    uint32_t rng = 12345u;
    float rnd() { rng = rng * 1664525u + 1013904223u; return (float) (rng >> 8) * (1.f / 16777216.f) * 2.f - 1.f; }
    void reset (float ph = 0.f) { phase = ph; sh = rnd(); }
    float tick (int wave, float inc)
    {
        float o;
        switch (wave)
        {
            case 0:  o = std::sin (2.f * kPi * phase); break;
            case 1:  o = phase < 0.5f ? 4.f * phase - 1.f : 3.f - 4.f * phase; break;
            case 2:  o = 2.f * phase - 1.f; break;
            case 3:  o = 1.f - 2.f * phase; break;
            case 4:  o = phase < 0.5f ? 1.f : -1.f; break;
            default: o = sh; break;
        }
        phase += inc;
        if (phase >= 1.f) { phase -= std::floor (phase); sh = rnd(); }
        return o;
    }
};

struct Smooth
{
    float v = 0.f, a = 0.f; bool init = false;
    void setup (float sr, float ms) { a = std::exp (-1.f / (ms * 0.001f * sr)); init = false; }
    float next (float t) { if (! init) { v = t; init = true; } v = t + a * (v - t); return v; }
};

// 31-tap half-band-style FIR used to decimate the 2x voice back to base rate.
struct Decimator
{
    static constexpr int N = 31;
    float h[N] {}, buf[N] {};
    int pos = 0;
    void init()
    {
        const int M = (N - 1) / 2; float sum = 0.f;
        for (int n = 0; n < N; ++n)
        {
            const float x = (float) (n - M);
            const float s = x == 0.f ? 1.f : std::sin (kPi * 0.5f * x) / (kPi * 0.5f * x);
            const float w = 0.42f - 0.5f * std::cos (2.f * kPi * n / (N - 1)) + 0.08f * std::cos (4.f * kPi * n / (N - 1));
            h[n] = 0.5f * s * w; sum += h[n];
        }
        for (auto& c : h) c /= sum;
        std::fill (buf, buf + N, 0.f); pos = 0;
    }
    void push (float x) { buf[pos] = x; pos = (pos + 1) % N; }
    float out() const
    {
        float a = 0.f; int idx = pos;
        for (int k = 0; k < N; ++k) { idx = idx == 0 ? N - 1 : idx - 1; a += h[k] * buf[idx]; }
        return a;
    }
};

// ---------------------------------------------------------------------------
// The single monophonic voice. The whole chain runs at 2x sample rate and is
// decimated at the end, which handles aliasing from the triangle and drives.
//   Osc -> Filter1 -> Drive1 -> Filter2 -> Drive2 -> DC block -> VCA -> Out
// ---------------------------------------------------------------------------
class Voice
{
public:
    void prepare (double sampleRate)
    {
        sr1 = (float) sampleRate; sr2 = sr1 * 2.f;
        for (auto* s : { &smShape, &smF1, &smF2, &smD1, &smD2, &smOut }) s->setup (sr2, 5.f);
        smVca.setup (sr2, 0.7f); smVca.v = 0.f; smVca.init = true;
        dcR = 1.f - 2.f * kPi * 5.f / sr2;
        decim.init();
        svf1.reset(); svf2.reset();
        dcX = dcY = 0.f; phase = 0.f;
        kill();
    }

    void setParams (const Params& np)
    {
        p = np;
        tF1 = std::log2 (std::max (20.f, p.f1Cut));
        tF2 = std::log2 (std::max (20.f, p.f2Cut));
    }

    void noteOn (int note, float vel)
    {
        noteHz = 440.f * std::exp2 ((float) (note - 69) / 12.f);
        velAmp = (1.f - p.velSens) + p.velSens * vel;
        phase = 0.f;
        vcaEnv.pos = pitchEnv.pos = f1Env.pos = 0.f;
        if (p.lfoMode == 0) lfo.reset (0.f);
    }

    void kill()                       { vcaEnv.pos = pitchEnv.pos = f1Env.pos = 1.f; }
    void setBend (float semis)        { bendSemis = semis; }
    void setModWheel (float v)        { modWheel = v; }
    void setTap (int t)               { tapSel = std::clamp (t, 0, 5); }
    void setLfoPhase (float ph)       { lfo.phase = ph - std::floor (ph); }

    // One base-rate sample. 'tap' receives the selected scope tap.
    float process (float& tap)
    {
        float l = lfo.tick (p.lfoWave, p.lfoHz / sr1);
        if (p.lfoUni) l = 0.5f * (l + 1.f);
        l *= p.lfoDepth;

        float tv = 0.f;
        for (int i = 0; i < 2; ++i)
        {
            decim.push (tick2x (l));
            if (i == 0) tv = taps[tapSel];
        }
        tap = tv;
        return decim.out();
    }

private:
    float tick2x (float l)
    {
        // ---- envelopes (all simple decays) ----
        const float vcaE = vcaEnv.tick (p.vcaMs * 0.001f * sr2, p.vcaCurve * 8.f);
        const float f1E  = p.f1Link ? vcaE : f1Env.tick (p.f1Ms * 0.001f * sr2, p.f1Curve * 8.f);

        // Pitch envelope with hard limits applied to the FINAL values.
        const float pDecay = std::max (p.pMs, kMinPitchDecayMs);
        const float depth  = std::clamp (p.pDepth, kMinPitchDepthSemis, kMaxPitchDepthSemis);
        const float pE     = pitchEnv.tick (pDecay * 0.001f * sr2, std::clamp (p.pCurve, 0.f, 1.f) * 3.f);

        // ---- oscillator ----
        float base = p.keyTrack ? noteHz : p.endHz;
        base = std::clamp (base, 10.f, 2000.f);
        const float semis = depth * pE + bendSemis + l * p.lfoPitch;
        const float freq  = std::min (base * std::exp2 (semis / 12.f), sr2 * 0.45f);
        phase += freq / sr2;
        if (phase >= 1.f) phase -= 1.f;

        const float shape = std::clamp (smShape.next (p.shape) + p.shapeEnv * vcaE + l * p.lfoShape, 0.f, 1.f);
        const float sine  = std::sin (2.f * kPi * phase);
        const float tri   = phase < 0.25f ? 4.f * phase : (phase < 0.75f ? 2.f - 4.f * phase : 4.f * phase - 4.f);
        const float osc   = sine + shape * (tri - sine);
        taps[0] = osc;

        // ---- Filter 1 (envelope + LFO modulated) ----
        const float c1 = std::exp2 (smF1.next (tF1) + p.f1Amt * f1E + l * p.lfoF1);
        float x = svf1.process (osc, c1, p.f1Res, p.f1Mode, sr2);
        taps[1] = x;

        // ---- Drive 1 ----
        const float d1 = std::clamp (smD1.next (p.d1Drive) + l * p.lfoD1 + modWheel * 0.5f, 0.f, 1.f);
        x = saturate (x, p.d1Type, d1, p.d1Bias, p.d1Mix);
        taps[2] = x;

        // ---- Filter 2 ----
        const float c2 = std::exp2 (smF2.next (tF2) + l * p.lfoF2);
        x = svf2.process (x, c2, p.f2Res, p.f2Mode, sr2);
        taps[3] = x;

        // ---- Drive 2 ----
        const float d2 = std::clamp (smD2.next (p.d2Drive) + l * p.lfoD2, 0.f, 1.f);
        x = saturate (x, p.d2Type, d2, p.d2Bias, p.d2Mix);
        taps[4] = x;

        // ---- DC blocker (~5 Hz) ----
        const float y = x - dcX + dcR * dcY;
        dcX = x; dcY = y;

        // ---- VCA: smoothed (~0.7 ms) so retriggers never click ----
        const float target = std::max (0.f, vcaE * velAmp * (1.f + l * p.lfoVca));
        const float gain   = smVca.next (target);
        float out = y * gain * smOut.next (p.outGain);

        // ---- transparent soft limiter ----
        const float a = std::fabs (out), t = 0.9f;
        if (a > t) out = std::copysign (t + (1.f - t) * std::tanh ((a - t) / (1.f - t)), out);
        taps[5] = out;
        return out;
    }

    Params p;
    float sr1 = 44100.f, sr2 = 88200.f;
    float phase = 0.f, noteHz = 48.f, velAmp = 1.f, bendSemis = 0.f, modWheel = 0.f;
    float tF1 = 14.f, tF2 = 14.f, dcR = 0.999f, dcX = 0.f, dcY = 0.f;
    int tapSel = 5;
    float taps[6] {};
    DecayEnv vcaEnv, pitchEnv, f1Env;
    Svf svf1, svf2;
    Lfo lfo;
    Smooth smShape, smF1, smF2, smD1, smD2, smOut, smVca;
    Decimator decim;
};

} // namespace mk
