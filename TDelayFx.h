/* -*- mode: c++ -*- */
#pragma once

#include "filters.h"
#include "TBaseEffect.h"
#include "TWaveShaper.h"
#include "util.h"

class TDelayFx: public TBaseEffect
{
UNCOPYABLE(TDelayFx)
    ;
public:
    TDelayFx();
    virtual void Process(TSampleBufferCollection& in, TSampleBufferCollection& out);

    void SetDelay(unsigned ms)
    {
        Delay = clamp(ms_to_samples(ms), 1, int(BufferSize) - 1);
    }
    void SetFeedback(TFraction f)
    {
        Feedback = f;
    }
    void SetDistortion(TFraction depth)
    {
        Distortion.SetDepth(depth);
    }
    void SetInsertGain(TFraction gain)
    {
        InsertGain = gain;
    }
    void SetLfo(unsigned depthMs, float speedHz, TFraction phase)
    {
        LfoDepth = ms_to_samples(depthMs);
        LfoSpeed = speedHz / TGlobal::SampleRate * 2 * M_PI;
        LfoChannelPhase = phase;
    }

private:
    static const size_t BufferSize = 0x40000;
    TSample Buffer[2][BufferSize];
    unsigned Delay; ///< Delay in samples
    TFraction Feedback;
    int WritePos;
    TOnePoleLpFilter Filter[2];
    TWaveShaper Distortion;
    int LfoDepth; ///< Depth of LFO modulation, in samples
    float LfoSpeed;
    TFraction LfoChannelPhase; ///< Difference in modulation phase between left and right
    double LfoPhaseAccumulator;
    TFraction InsertGain;
    TFraction Pingpong; ///< Stereo factor. 0.5 is neutral and will cause a mono-downmix in the buffer.
};
