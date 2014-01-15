/* -*- mode: c++ -*- */
#pragma once

#include <cmath>
#include "TBaseOscillator.h"
#include "TGlobal.h"

class TInputOscillator: public TBaseOscillator
{
public:
    TInputOscillator(const TNoteData& noteData)
        : TBaseOscillator(noteData)
    {
    }

    void Process(TSampleBuffer& in, TSampleBuffer& out, TSampleBuffer& syncin,
            TSampleBuffer& syncout)
    {
        out.AddSamples(in);
        syncout.Clear();
    }
};
