#include "TSvfFilter.h"
#include "TGlobal.h"
#include <cassert>

TSvfFilter::TSvfFilter()
: Coeffs {.f = 0.1f, .q = 0.0f}, State1 {0, 0}, State2 {0, 0}, Mix {1.0f, 0.0f, 0.0f}, WaveShaper(1.0f)
{
    // Can't set cutoff, because sample rate may be unknown
    SetResonance(0.0f);
}

void TSvfFilter::SetCutoff(int cutoff)
{
    float fcutoff = float(cutoff) / (2.0f * TGlobal::NyquistFrequency);
    fcutoff = clamp(fcutoff, 0.001f, 0.48f);
    fcutoff /= Oversample;
    Coeffs.f = 2.0f * sinf(M_PI * fcutoff);
}

void TSvfFilter::SetResonance(TFraction resonance)
{
    float q = 1.0f + resonance * 5;
    Coeffs.q = 1.0f / q;
}

void TSvfFilter::Crunch(const TCoeffs coeffs, TSample* state, const TSample& ins, TSample& lp,
        TSample& hp, TSample& bp)
{
    const float clamping = 5.0f;

    lp = state[0] * coeffs.f + state[1];
    state[1] = clamp(lp, -clamping, clamping);
    hp = ins - lp - coeffs.q * state[0];
    bp = hp * coeffs.f + state[0];
    state[0] = clamp(bp, -clamping, clamping);
}

void TSvfFilter::Process(TSampleBuffer& in, TSampleBuffer& out)
{
    std::transform(in.begin(), in.end(), out.begin(), [this](const TSample& ins) -> TSample {
        TSample lp, hp, bp;
        for (int i=0; i < Oversample; i++) {
            Crunch(Coeffs, State1, ins, lp, hp, bp);
        }
        return WaveShaper.Tanh(Mix[0] * lp + Mix[1] * hp + Mix[2] * bp);
    });
    std::transform(out.begin(), out.end(), out.begin(), [this](const TSample& ins) -> TSample {
        TSample lp, hp, bp;
        for (int i=0; i < Oversample; i++) {
            Crunch(Coeffs, State2, ins, lp, hp, bp);
        }
        return WaveShaper.Tanh(Mix[0] * lp + Mix[1] * hp + Mix[2] * bp);
    });
}
