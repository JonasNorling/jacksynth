/* -*- mode: c++ -*- */
#pragma once

#include <map>
#include "IAudioPort.h"
#include "iir.h"
#include "util.h"

class TButterworthLpFilter
{
UNCOPYABLE(TButterworthLpFilter)
    ;

public:
    TButterworthLpFilter();

    void SetCutoff(int cutoff);
    void Process(TSampleBuffer& in, TSampleBuffer& out);

private:
    // Filter order. N=2 --> 12dB/oct, N=4 --> 24dB/oct.
    static const int N = 4;

    struct TCoeffs
    {
        TSample C[N + 1];
        TSample D[N + 1];
        TSample Gain;
    } Coeffs;

    TSample X[N + 1];
    TSample Y[N + 1];

    float Cutoff;

    typedef std::map<int, TCoeffs> TCache;
    static TCache Cache;
};
