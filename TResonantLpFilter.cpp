#include "TResonantLpFilter.h"
#include "TGlobal.h"
#include <cassert>

/*
 * This filter gives a swooshy resonant effects when Resonance > 10.
 * FIXME: When dumping the cutoff quickly the filter can pop very
 * loudly.
 *
 * Nicked from the musicdsp.org code archive:
 Another 4-pole lowpass...

 Type : 4-pole LP/HP
 References : Posted by fuzzpilz [AT] gmx [DOT] net

 Notes :
 Vaguely based on the Stilson/Smith Moog paper, but going in a rather different direction from others I've seen here.
 */
TResonantLpFilter::TResonantLpFilter()
        : Cutoff(-1), Resonance(10)
{
    std::fill(D, D + N, 0);
}

void TResonantLpFilter::SetResonance(TFraction resonance)
{
    float q = 1 + resonance * 20;
    if (Resonance == q) {
        return;
    }
    Resonance = q;
    CalculateCoeffs();
}

void TResonantLpFilter::SetCutoff(int cutoff)
{
    float fcutoff = float(cutoff) / TGlobal::NyquistFrequency;
    fcutoff = clamp(fcutoff, 0.01f, 0.95f);
    if (fcutoff == Cutoff) {
        return;
    }
    Cutoff = fcutoff; // 0..1
    CalculateCoeffs();
}

void TResonantLpFilter::CalculateCoeffs()
{
    double omega = Cutoff * M_PI; //peak freq
    double g = Resonance; //peak mag

    // calculating coefficients:

    double k, p, q, a;
    double a0, a1, a2, a3, a4;

    k = (4.0 * g - 3.0) / (g + 1.0);
    p = 1.0 - 0.25 * k;
    p *= p;

    // LP:
    a = 1.0 / (tan(0.5 * omega) * (1.0 + p));
    p = 1.0 + a;
    q = 1.0 - a;

    a0 = 1.0 / (k + p * p * p * p);
    a1 = 4.0 * (k + p * p * p * q);
    a2 = 6.0 * (k + p * p * q * q);
    a3 = 4.0 * (k + p * q * q * q);
    a4 = (k + q * q * q * q);
    p = a0 * (k + 1.0);

    Coef[0] = p;
    Coef[1] = 4.0 * p;
    Coef[2] = 6.0 * p;
    Coef[3] = 4.0 * p;
    Coef[4] = p;
    Coef[5] = -a1 * a0;
    Coef[6] = -a2 * a0;
    Coef[7] = -a3 * a0;
    Coef[8] = -a4 * a0;

#if 0
    fprintf(stderr, "Resonant LP, cutoff %.3f*pi, resonance %.3f:\n", Cutoff, Resonance);
    fprintf(stderr, "  Coeff = ");
    for (int i=0; i<=N; i++) {
        fprintf(stderr, "%.3f ", Coef[i]);
    }
    fprintf(stderr, "\n          ");
    for (int i=N+1; i<=N*2; i++) {
        fprintf(stderr, "%.3f ", Coef[i]);
    }
    fprintf(stderr, "\n");
#endif
}

void TResonantLpFilter::Process(TSampleBuffer& in, TSampleBuffer& out)
{
    std::transform(in.begin(), in.end(), out.begin(), [this](const TSample& in) -> TSample {
        TSample out = Coef[0]*in+D[0];
        D[0] = Coef[1]*in + Coef[5]*out + D[1];
        D[1] = Coef[2]*in + Coef[6]*out + D[2];
        D[2] = Coef[3]*in + Coef[7]*out + D[3];
        D[3] = Coef[4]*in + Coef[8]*out;
        return out;
    });
}
