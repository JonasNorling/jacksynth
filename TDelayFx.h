/* -*- mode: c++ -*- */
#pragma once

#include "IEffect.h"

class TDelayFx: public IEffect
{
UNCOPYABLE(TDelayFx)
    ;
public:
    TDelayFx();
    virtual void Process(TSampleBufferCollection& in,
            TSampleBufferCollection& out);

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
};
