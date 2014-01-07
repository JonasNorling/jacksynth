/* -*- mode: c++ -*- */
#pragma once

#include "util.h"
#include "IAudioPort.h"
#include "TEnvelope.h"

class IOscillator
{
UNCOPYABLE(IOscillator)
    ;

public:
    IOscillator()
    {
    }
    virtual ~IOscillator()
    {
    }

    virtual void SetFrequency(TFrequency hz) = 0;
    virtual void SetPulseWidth(float pw) = 0;
    virtual void SetSync(bool sync) = 0;
    virtual void SetState(TEnvelope::TState state) = 0;
    virtual void Process(TSampleBuffer& in, TSampleBuffer& out,
            TSampleBuffer& syncin, TSampleBuffer& syncout) = 0;
};

class TNoOscillator: public IOscillator
{
public:
    TNoOscillator()
    {
    }
    virtual ~TNoOscillator()
    {
    }

    void SetFrequency(TFrequency)
    {
    }
    void SetPulseWidth(float)
    {
    }
    void SetSync(bool sync)
    {
    }
    void SetState(TEnvelope::TState state)
    {
    }

    void Process(TSampleBuffer&, TSampleBuffer&, TSampleBuffer& syncin,
            TSampleBuffer& syncout)
    {
    }
};
