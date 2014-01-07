/* -*- mode: c++ -*- */
#pragma once

#include "util.h"
#include "IAudioPort.h"
#include "TEnvelope.h"

class TBaseOscillator
{
UNCOPYABLE(TBaseOscillator)
    ;

public:
    TBaseOscillator()
        : Hz(0), Sync(false), State(TEnvelope::IDLE), PhaseAccumulator(0.0f)
    {
    }

    virtual ~TBaseOscillator()
    {
    }

    virtual void SetFrequency(TFrequency hz)
    {
        Hz = hz;
    }

    virtual void SetPulseWidth(float pw)
    {
    }

    virtual void SetSync(bool sync)
    {
        Sync = sync;
    }

    virtual void SetState(TEnvelope::TState state)
    {
        State = state;
    }

    virtual void Process(TSampleBuffer& in, TSampleBuffer& out,
            TSampleBuffer& syncin, TSampleBuffer& syncout)
    {
    }

protected:
    double Hz;
    bool Sync;
    TEnvelope::TState State;
    double PhaseAccumulator;
};
