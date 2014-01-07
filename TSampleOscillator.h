/* -*- mode: c++ -*- */
#pragma once

#include <memory>
#include "IOscillator.h"
#include "TEnvelope.h"
#include "TGlobal.h"

class TSampleOscillator: public IOscillator
{
UNCOPYABLE(TSampleOscillator)
    ;

public:
    TSampleOscillator()
            : Hz(0), Scanpos(0), Scanspeed(0), LoopPoint1(0), LoopPoint2(-1), State(TEnvelope::IDLE), Sample()
    {
    }
    void SetFrequency(TFrequency hz)
    {
        Hz = hz;
    }
    void SetPulseWidth(float)
    {
    }
    void SetSync(bool sync)
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
    double Hz;
    double Scanpos;
    double Scanspeed;
    int LoopPoint1;
    int LoopPoint2;
    TEnvelope::TState State;
    TSampleBuffer* Sample;
};
