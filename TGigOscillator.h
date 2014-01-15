/* -*- mode: c++ -*- */
#pragma once

#include <memory>
#include "TBaseOscillator.h"
#include "TGigInstrument.h"

class TGigOscillator: public TBaseOscillator
{
UNCOPYABLE(TGigOscillator)
    ;

public:
    TGigOscillator(const TNoteData& noteData)
        : TBaseOscillator(noteData), Instrument(NULL), Region(NULL), Sample(NULL), SampleData(), UnityHz(0)
    {
    }

    void SetInstrument(TGigInstrument& instrument);

    void Process(TSampleBuffer& in, TSampleBuffer& out,
            TSampleBuffer& syncin, TSampleBuffer& syncout);

private:
    TGigInstrument* Instrument;
    gig::Region* Region;
    gig::Sample* Sample;
    gig::buffer_t SampleData;
    float UnityHz;
};
