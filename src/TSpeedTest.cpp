#include "TSpeedTest.h"
#include "IAudioPort.h"
#include "TButterworthLpFilter.h"
#include "util.h"

TSpeedTest::TSpeedTest()
{
}

static void fopt(const TSample* in, TSample* out, unsigned count)
{
    static float d = 0.0f;

    for (unsigned i = 0; i < count; i++) {
        out[i] = in[i] + 0.4f*d;
    }
}

static void butterworth(const TSample* __restrict__ in,
        TSample* __restrict__ out, unsigned count)
{
    static const int N = 4;

    struct TCoeffs
    {
        TSample C[N + 1];
        TSample D[N + 1];
        TSample Gain;
    } Coeffs;

    Coeffs = { { 1,2,3,4,5 }, { 1,2,3,4,5 }, 0.1 };

    static TSample X[N + 1];
    static TSample Y[N + 1];

    for (unsigned i = 0; i < count; i++) {
        for (int n=N; n>0; n--) {
            X[n] = X[n-1];
            Y[n] = Y[n-1];
        }
        X[0] = in[i] * Coeffs.Gain;
        Y[0] = 0;
        for (int n=0; n<=N; n++) {
            Y[0] += Coeffs.C[n]*X[n] - Coeffs.D[n]*Y[n];
        }
        out[i] = Y[0];
    }
}

void TSpeedTest::Run()
{
    const unsigned COUNT = 1000000;
    const unsigned RUNS = 10;
    const unsigned SLICE_SIZE = 32;

    TSample* sampledata1 = new TSample[COUNT];
    TSample* sampledata2 = new TSample[COUNT];
    TSampleBuffer buf1(sampledata1, COUNT);
    TSampleBuffer buf2(sampledata2, COUNT);

    TButterworthLpFilter lp;
    lp.SetCutoff(0.5);

    // Load cache
    lp.Process(buf1, buf2);
    lp.Process(buf1, buf2);
    lp.Process(buf1, buf2);

    TTimer timerFopt1("fopt");
    for (unsigned run = 0; run < RUNS; run++) {
        timerFopt1.Start();
        fopt(sampledata1, sampledata2, COUNT);
        timerFopt1.Stop();
    }

    TTimer timerBig("butterworth unsliced");
    for (unsigned run = 0; run < RUNS; run++) {
        timerBig.Start();
        lp.Process(buf1, buf2);
        timerBig.Stop();
    }

    TTimer timerSmall("butterworth sliced");
    for (unsigned run = 0; run < RUNS; run++) {
        timerSmall.Start();
        for (unsigned slice = 0; slice < COUNT; slice += SLICE_SIZE) {
            TSampleBuffer slice1(buf1.Slice(slice, slice + SLICE_SIZE));
            TSampleBuffer slice2(buf2.Slice(slice, slice + SLICE_SIZE));
            lp.Process(slice1, slice2);
        }
        timerSmall.Stop();
    }

    TTimer timerLocal("butterworth local");
    for (unsigned run = 0; run < RUNS; run++) {
        timerLocal.Start();
        butterworth(sampledata1, sampledata2, COUNT);
        timerLocal.Stop();
    }
}
