/* -*- mode: c++ -*- */
#pragma once

#include <algorithm>
#include "IAudioPort.h"
#include "util.h"

class TFirstrOrderFilterKernel
{
    UNCOPYABLE(TFirstrOrderFilterKernel)
    ;

public:
    TFirstrOrderFilterKernel(TSample a0, TSample a1, TSample b1)
    {
        X[0] = 0.0f;
        Y[0] = 0.0f;
        a[0] = a0;
        a[1] = a1;
        b[0] = 0.0f;
        b[1] = b1;
    }

    virtual ~TFirstrOrderFilterKernel() {}

    void Process(TSampleBuffer& in, TSampleBuffer& out)
    {
        std::transform(in.begin(), in.end(), out.begin(),
                [this](const TSample& in) -> TSample { return ProcessOne(in); } );
    }

    TSample ProcessOne(const TSample& x)
    {
        TSample y = a[0] * x + a[1] * X[0] + b[1] * Y[0];
        X[0] = x;
        Y[0] = y;
        return y;
    }

protected:
    TSample X[1];
    TSample Y[1];
    TSample a[2]; // Coeffs pertaining to X (input)
    TSample b[2]; // Coeffs pertaining to Y (output). b[0] unused.
};

class TDcBlocker : public TFirstrOrderFilterKernel
{
public:
    TDcBlocker()
        : TFirstrOrderFilterKernel(1.0f, -1.0f, 0.95f)
    {
    }
};

class TOnePoleLpFilter : public TFirstrOrderFilterKernel
{
public:
    TOnePoleLpFilter(TSample initValue = 0.0f)
        : TFirstrOrderFilterKernel(1.0f, 0, 0)
    {
        X[0] = initValue;
        Y[0] = initValue;
    }

    /**
     * @param x 0..1
     */
    void SetPole(TSample x)
    {
        a[0] = 1.0f - x;
        b[1] = x;
    }

    /**
     * Cutoff as a fraction of sample rate.
     * @param fc 0..0.5
     */
    void SetCutoff(TFraction fc)
    {
        TSample x = expf(-2.0f * M_PI * fc);
        SetPole(x);
    }

    void SetCutoffHz(int fchz)
    {
        SetCutoff(float(fchz) / TGlobal::SampleRate);
    }
};
