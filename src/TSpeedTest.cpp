#include "TSpeedTest.h"
#include "IAudioPort.h"
#include "TButterworthLpFilter.h"
#include "TMinBlepPulseOscillator.h"
#include "TMinBlepSawOscillator.h"
#include "TWavetableOscillator.h"
#include "util.h"

TSpeedTest::TSpeedTest()
{
}

static void __attribute__((noinline)) fopt(const TSample* in, TSample* out, unsigned count)
{
    float d = 1.1f;
    float a = in[0];

    // This loop is fairly slow, probably because of the data dependence on
    // the last iteration.

    for (unsigned i = 0; i < count; i++) {
        d *= a;
    }

    out[0] = d;
}

static void __attribute__((noinline)) foptOnData(const TSample* in, TSample* out, unsigned count)
{
    static float d = 0.0f;

    for (unsigned i = 0; i < count; i++) {
        out[i] = in[i] * d;
        d = in[i];
    }
}

static void __attribute__((noinline)) foptRestrict(
        const TSample* __restrict__ in,
        TSample* __restrict__ out,
        unsigned count)
{
    static float d = 0.0f;

    for (unsigned i = 0; i < count; i++) {
        out[i] = in[i] * d;
        d = in[i];
    }
}

static void __attribute__((noinline)) butterworth(const TSample* __restrict__ in,
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

static void __attribute__((noinline)) butterworthOnBuffer(const TSampleBuffer& in,
        TSampleBuffer& out)
{
    const size_t count = in.GetCount();
    const TSample* __restrict__ ins = in.begin();
    TSample* __restrict__ outs = out.begin();

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
        X[0] = ins[i] * Coeffs.Gain;
        Y[0] = 0;
        for (int n=0; n<=N; n++) {
            Y[0] += Coeffs.C[n]*X[n] - Coeffs.D[n]*Y[n];
        }
        outs[i] = Y[0];
    }
}

void TSpeedTest::Run()
{
    const unsigned COUNT = 1000000;
    const unsigned RUNS = 10;
    const unsigned SLICE_SIZE = 32;

    TSample* sampledata1 = new TSample[COUNT];
    TSample* sampledata2 = new TSample[COUNT];
    TSample* sampledata3 = new TSample[COUNT];
    TSample* sampledata4 = new TSample[COUNT];
    TSampleBuffer buf1(sampledata1, COUNT);
    TSampleBuffer buf2(sampledata2, COUNT);
    TSampleBuffer buf3(sampledata2, COUNT);
    TSampleBuffer buf4(sampledata2, COUNT);

    std::fill(&sampledata1[0], &sampledata1[COUNT], 1.0f);
    std::fill(&sampledata2[0], &sampledata2[COUNT], 1.0f);
    std::fill(&sampledata3[0], &sampledata3[COUNT], 1.0f);
    std::fill(&sampledata4[0], &sampledata4[COUNT], 1.0f);

    TButterworthLpFilter lp;
    lp.SetCutoff(3000);

    // Load cache
    lp.Process(buf1, buf2);

    TTimer timerFopt("fopt");
    for (unsigned run = 0; run < RUNS; run++) {
        timerFopt.Start();
        fopt(sampledata1, sampledata2, COUNT);
        timerFopt.Stop();
    }

    TTimer timerFoptOnData("foptOnData");
    for (unsigned run = 0; run < RUNS; run++) {
        timerFoptOnData.Start();
        foptOnData(sampledata1, sampledata2, COUNT);
        timerFoptOnData.Stop();
    }

    TTimer timerFoptRestrict("foptRestrict");
    for (unsigned run = 0; run < RUNS; run++) {
        timerFoptRestrict.Start();
        foptRestrict(sampledata1, sampledata2, COUNT);
        timerFoptRestrict.Stop();
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

    TTimer timerLocalBuffer("butterworth local buffer");
    for (unsigned run = 0; run < RUNS; run++) {
        timerLocalBuffer.Start();
        butterworthOnBuffer(buf1, buf2);
        timerLocalBuffer.Stop();
    }

    TTimer timerOsc1("minBLEP saw");
    TMinBlepSawOscillator minBlepSaw({TGlobal::MidiNoteE3, 64});
    minBlepSaw.SetFrequency(NOTE2HZ(TGlobal::MidiNoteE3));
    for (unsigned run = 0; run < RUNS; run++) {
        timerOsc1.Start();
        minBlepSaw.Process(buf1, buf2, buf3, buf4);
        timerOsc1.Stop();
    }

    TTimer timerOsc2("minBLEP pulse");
    TMinBlepPulseOscillator minBlepPulse({TGlobal::MidiNoteE3, 64});
    minBlepPulse.SetFrequency(NOTE2HZ(TGlobal::MidiNoteE3));
    for (unsigned run = 0; run < RUNS; run++) {
        timerOsc2.Start();
        minBlepPulse.Process(buf1, buf2, buf3, buf4);
        timerOsc2.Stop();
    }

    TTimer timerOsc3("wavetable");
    TWavetableOscillator wavetable({TGlobal::MidiNoteE3, 64});
    wavetable.SetFrequency(NOTE2HZ(TGlobal::MidiNoteE3));
    for (unsigned run = 0; run < RUNS; run++) {
        timerOsc3.Start();
        wavetable.Process(buf1, buf2, buf3, buf4);
        timerOsc3.Stop();
    }

    printf("\n");
    timerFopt.ReportOpsPerSecond(COUNT);
    timerFoptOnData.ReportOpsPerSecond(COUNT);
    timerFoptRestrict.ReportOpsPerSecond(COUNT);
    timerBig.ReportOpsPerSecond(COUNT);
    timerSmall.ReportOpsPerSecond(COUNT);
    timerLocal.ReportOpsPerSecond(COUNT);
    timerLocalBuffer.ReportOpsPerSecond(COUNT);
    timerOsc1.ReportOpsPerSecond(COUNT);
    timerOsc2.ReportOpsPerSecond(COUNT);
    timerOsc3.ReportOpsPerSecond(COUNT);
    printf("\n");
}
