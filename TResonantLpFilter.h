/* -*- mode: c++ -*- */
#pragma once

#include <map>
#include "IAudioPort.h"
#include "util.h"

class TResonantLpFilter
{
UNCOPYABLE(TResonantLpFilter)
    ;

public:
    TResonantLpFilter();

    void SetCutoff(int cutoff);
    void SetResonance(TFraction resonance);
    void Process(TSampleBuffer& in, TSampleBuffer& out);

private:
    void CalculateCoeffs();

    static const int N = 4;

    TSample Coef[N + N + 1];
    TSample D[N];

    float Cutoff;
    float Resonance;
};
