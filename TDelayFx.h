/* -*- mode: c++ -*- */
#pragma once

#include "filters.h"
#include "TBaseEffect.h"
#include "TWaveShaper.h"

class TDelayFx: public TBaseEffect
{
UNCOPYABLE(TDelayFx)
    ;
public:
    TDelayFx();
    virtual void Process(TSampleBufferCollection& in, TSampleBufferCollection& out);

    void SetDelay(unsigned ms)
    {
        Delay = ms_to_samples(ms);
    }
    void SetFeedback(TFraction f)
    {
        Feedback = f;
    }
    void SetDistortion(TFraction depth)
    {
        Distortion.SetDepth(depth);
    }

private:
    static const size_t BufferSize = 0x10000;
    TSample Buffer[2][BufferSize];
    unsigned Delay; // in samples
    TFraction Feedback;
    int ReadPos;
    TOnePoleLpFilter Filter[2];
    TWaveShaper Distortion;
};
