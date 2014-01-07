/* -*- mode: c++ -*- */
#pragma once

#include "util.h"
#include "IAudioPort.h"

class IEffect
{
UNCOPYABLE(IEffect)
    ;

public:
    IEffect()
    {
    }
    virtual ~IEffect()
    {
    }
    virtual void Process(TSampleBufferCollection&,
            TSampleBufferCollection&) = 0;
};

class TNoEffect: public IEffect
{
public:
    TNoEffect()
    {
    }
    virtual ~TNoEffect()
    {
    }

    void SetPulseWidth(float)
    {
    }

    virtual void Process(TSampleBufferCollection&, TSampleBufferCollection&)
    {
    }
};
