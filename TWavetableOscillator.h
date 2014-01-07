/* -*- mode: c++ -*- */
#pragma once

#include <cmath>
#include <algorithm>
#include "util.h"
#include "IOscillator.h"
#include "TGlobal.h"

class TWavetableOscillator: public IOscillator
{
UNCOPYABLE(TWavetableOscillator)
    ;

public:
    static const int Wavelength = 256 * 4;

    TWavetableOscillator();
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
        for (TSample& outs : out) {
            outs = linterpolate(Wave, Wavelength, Scanpos);
            Scanpos += (Hz * Wavelength) / TGlobal::SampleRate;
            if (Scanpos >= Wavelength) Scanpos -= Wavelength;
        }
        syncout.Clear();
    }

private:
    double Hz;
    double Scanpos; // 0..Wavelength

    const TSample* Wave;

    static const std::vector<TSample> SineTable;
    static const std::vector<TSample> SquareTable;
};
