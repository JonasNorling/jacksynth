/* -*- mode: c++ -*- */
#pragma once

#include "TBaseEffect.h"

class TDelayFx: public TBaseEffect
{
UNCOPYABLE(TDelayFx)
    ;
public:
    TDelayFx();
    virtual void Process(TSampleBufferCollection& in, TSampleBufferCollection& out);

    void SetDelay(int ms)
    {
        Delay = ms_to_samples(ms);
    }
    void SetFeedback(TFraction f)
    {
        Feedback = f;
    }

private:
    static const size_t BufferSize = 48000;
    TSample Buffer[2][BufferSize];
    int Delay; // in samples
    TFraction Feedback;
    int ReadPos;
    TSample FilterData[2];
};
