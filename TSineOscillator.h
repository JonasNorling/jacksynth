/* -*- mode: c++ -*- */
#pragma once

#include <cmath>
#include "IOscillator.h"
#include "TGlobal.h"

class TSineOscillator: public IOscillator
{
public:
    TSineOscillator()
            : Hz(0), Scanpos(0)
    {
    }
    void SetFrequency(TFrequency hz)
    {
        Hz = hz;
    }
    void SetPulseWidth(float)
    {
    }
    void SetSync(bool sync)
    {
    }
    void SetState(TEnvelope::TState state)
    {
    }

    void Process(TSampleBuffer& in, TSampleBuffer& out, TSampleBuffer& syncin,
            TSampleBuffer& syncout)
    {
        // FIXME: This is quite slow for some reason. Also, don't
        // increment scanpos for ever, it's going to lose precision.
        for (TSample& outs : out) {
            outs = TGlobal::OscAmplitude * sinf(Scanpos);
            Scanpos += Hz / TGlobal::SampleRate * 2 * M_PI;
        }
        syncout.Clear();
    }

private:
    double Hz;
    double Scanpos;
};
