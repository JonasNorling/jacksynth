/* -*- mode: c++ -*- */
#pragma once

#include <cmath>
#include "IAudioPort.h"
#include "TGlobal.h"

/**
 * Trivial pulse (square) oscillator that will cause a ton of evil aliasing.
 */
class TPulseOscillator
{
public:
    TPulseOscillator()
            : Hz(0), Scanpos(0), Out(0)
    {
    }
    void SetFrequency(double hz)
    {
        Hz = hz;
    }
    void SetPulseWidth(float)
    {
    }

    void Process(TSampleBuffer& in, TSampleBuffer& out)
    {
        for (TSample& outs : out) {
            Scanpos += Hz / TGlobal::SampleRate;
            if (Scanpos >= 1) Scanpos -= 1;

            const float A = TGlobal::OscAmplitude;
            const float pw = 0.5;

            outs = Scanpos > pw ? A : -A;
        }
    }

private:
    double Hz;
    double Scanpos;
    TSample Out;
};
