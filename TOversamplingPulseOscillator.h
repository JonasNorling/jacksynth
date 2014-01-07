/* -*- mode: c++ -*- */
#pragma once

#include <cmath>
#include "IAudioPort.h"
#include "TButterworthLpFilter.h"
#include "TGlobal.h"

class TOversamplingPulseOscillator
{
public:
    TOversamplingPulseOscillator(int oversample = 32)
            : Hz(0), Scanpos(0), Out(0), Oversample(oversample), Lp(
                    1.0 / Oversample)
    {
    }
    void SetFrequency(double hz)
    {
        Hz = hz;
    }

    void Process(TSampleBuffer& in, TSampleBuffer& out)
    {
        for (TSample& outs : out) {
            const float A = TGlobal::OscAmplitude;
            const float pw = 0.5;

            // Construct an oversampled pulse wave
            TSample data[Oversample];
            TSampleBuffer buffer(data, Oversample);
            for (TSample& overs : buffer) {
                // FIXME: This will probably drift and cause havoc
                Scanpos += Hz / TGlobal::SampleRate / Oversample;
                if (Scanpos >= 1) Scanpos -= 1;

                overs = Scanpos > pw ? A : -A;
            }

            // Low pass filter to nominal Nyquist rate
            Lp.Process(buffer, buffer);

            // Decimate back down to nominal sample rate
            outs = buffer[0];
        }
    }

private:
    double Hz;
    double Scanpos;
    TSample Out;
    int Oversample;
    TButterworthLpFilter<3> Lp;
};
