#include "TButterworthLpFilter.h"
#include "TGlobal.h"
#include <cassert>

TButterworthLpFilter::TCache TButterworthLpFilter::Cache;

TButterworthLpFilter::TButterworthLpFilter()
        : Coeffs(), Cutoff(-1)
{
    std::fill(X, X + N + 1, 0);
    std::fill(Y, Y + N + 1, 0);
}

/*
 * FIXME: The cutoff can be changed in "run time", and if that is done
 * slowly the glitches will be minimal. When jumping to a cutoff that
 * is significantly lower, the gain will make a huge positive jump
 * resulting in a pop in the output.
 */
void TButterworthLpFilter::SetCutoff(int cutoff)
{
    float fcutoff = float(cutoff) / TGlobal::NyquistFrequency;
    fcutoff = clamp(fcutoff, 0.01f, 0.95f);
    if (fcutoff == Cutoff) {
        return;
    }
    Cutoff = fcutoff;

    auto cached = Cache.find(cutoff);
    if (cached != Cache.end()) {
        Coeffs = cached->second;
        return;
    }

    double* d = dcof_bwlp(N, Cutoff);
    std::copy(&d[0], &d[N + 2], Coeffs.D);
    free(d);
    d = 0;
    int* c = ccof_bwlp(N);
    std::copy(&c[0], &c[N + 2], Coeffs.C);
    free(c);
    c = 0;
    Coeffs.Gain = sf_bwlp(N, Cutoff);

    Cache[cutoff] = Coeffs;

#if 0
    fprintf(stderr, "Butterworth LP of degree %d, cutoff %.3f*pi, gain compensation %e:\n",
            N, Cutoff, Coeffs.Gain);
    fprintf(stderr, "  D = ");
    for (int i=0; i<=N; i++) {
        fprintf(stderr, "%.3f ", Coeffs.D[i]);
    }
    fprintf(stderr, "\n  C = ");
    for (int i=0; i<=N; i++) {
        fprintf(stderr, "%.3f ", Coeffs.C[i]);
    }
    fprintf(stderr, "\n");
#endif
}

void TButterworthLpFilter::Process(TSampleBuffer& in, TSampleBuffer& out)
{
    std::transform(in.begin(), in.end(), out.begin(),
            [this](const TSample& ins) -> TSample {
                for (int n=N; n>0; n--) {
                    X[n] = X[n-1];
                    Y[n] = Y[n-1];
                }
                X[0] = ins * Coeffs.Gain;
                Y[0] = 0;
                for (int n=0; n<=N; n++) {
                    Y[0] += Coeffs.C[n]*X[n] - Coeffs.D[n]*Y[n];
                }
                return Y[0];
            });
}
