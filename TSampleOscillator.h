/* -*- mode: c++ -*- */
#pragma once

#include <memory>
#include "TBaseOscillator.h"
#include "TEnvelope.h"
#include "TGlobal.h"

class TSampleOscillator: public TBaseOscillator
{
UNCOPYABLE(TSampleOscillator)
    ;

public:
    TSampleOscillator(const TNoteData& noteData)
            : TBaseOscillator(noteData), Scanspeed(0), LoopPoint1(0), LoopPoint2(-1), State(TEnvelope::IDLE), Sample()
    {
    }
    void SetSample(TSampleBuffer* sample, double scanspeed)
    {
        Sample = sample;
        Scanspeed = scanspeed;
    }
    void SetLoopPoints(int point1, int point2)
    {
        LoopPoint1 = point1;
        LoopPoint2 = point2;
    }
    void SetState(TEnvelope::TState state)
    {
        State = state;
    }
    void Process(TSampleBuffer& in, TSampleBuffer& out, TSampleBuffer& syncin,
            TSampleBuffer& syncout);

private:
    double Scanspeed;
    int LoopPoint1;
    int LoopPoint2;
    TEnvelope::TState State;
    TSampleBuffer* Sample;
};
