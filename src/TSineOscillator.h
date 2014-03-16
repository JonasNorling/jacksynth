/* -*- mode: c++ -*- */
#pragma once

#include <cmath>
#include "TBaseOscillator.h"
#include "TGlobal.h"

class TSineOscillator: public TBaseOscillator
{
public:
    TSineOscillator(const TNoteData& noteData)
            : TBaseOscillator(noteData)
    {
    }

    void Process(TSampleBuffer& in, TSampleBuffer& out, TSampleBuffer& syncin,
            TSampleBuffer& syncout)
    {
        // FIXME: This is quite slow for some reason. Also, don't
        // increment scanpos for ever, it's going to lose precision.
        for (TSample& outs : out) {
            outs = TGlobal::OscAmplitude * sinf(PhaseAccumulator);
            PhaseAccumulator += Hz / TGlobal::SampleRate * 2 * M_PI;
        }
        syncout.Clear();
    }
};
