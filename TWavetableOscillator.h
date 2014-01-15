/* -*- mode: c++ -*- */
#pragma once

#include <cmath>
#include <algorithm>
#include "util.h"
#include "TBaseOscillator.h"
#include "TGlobal.h"

class TWavetableOscillator: public TBaseOscillator
{
UNCOPYABLE(TWavetableOscillator)
    ;

public:
    static const int Wavelength = 256 * 4;

    TWavetableOscillator(const TNoteData& noteData);

    void Process(TSampleBuffer& in, TSampleBuffer& out, TSampleBuffer& syncin,
            TSampleBuffer& syncout)
    {
        for (TSample& outs : out) {
            outs = linterpolate(Wave, Wavelength, PhaseAccumulator);
            PhaseAccumulator += (Hz * Wavelength) / TGlobal::SampleRate;
            if (PhaseAccumulator >= Wavelength) PhaseAccumulator -= Wavelength;
        }
        syncout.Clear();
    }

private:
    const TSample* Wave;

    static const std::vector<TSample> SineTable;
    static const std::vector<TSample> SquareTable;
};
