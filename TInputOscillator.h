/* -*- mode: c++ -*- */
#pragma once

#include <cmath>
#include "IOscillator.h"
#include "TGlobal.h"

class TInputOscillator: public IOscillator
{
public:
    TInputOscillator()
    {
    }
    void SetFrequency(TFrequency)
    {
    }
    void SetPulseWidth(float)
    {
    }
    void SetSync(bool sync)
    {
    }

    void Process(TSampleBuffer& in, TSampleBuffer& out, TSampleBuffer& syncin,
            TSampleBuffer& syncout)
    {
        out.AddSamples(in);
        syncout.Clear();
    }
};
