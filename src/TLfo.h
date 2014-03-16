/* -*- mode: c++ -*- */
#pragma once

#include "util.h"
#include "TGlobal.h"

class TLfo
{
UNCOPYABLE(TLfo)
    ;

public:
    TLfo(float hz = 0)
            : Hz(hz), Value(0), Scanpos(0)
    {
    }
    void SetFrequency(float hz)
    {
        Hz = hz;
    }

    void Step(int samples)
    {
        Value = sinf(Scanpos);
        Scanpos += samples * Hz / TGlobal::SampleRate * 2 * M_PI;
    }

    TFraction GetValue() const
    {
        return Value;
    }

private:
    float Hz;
    TFraction Value;
    double Scanpos;
};
