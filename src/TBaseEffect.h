/* -*- mode: c++ -*- */
#pragma once

#include "util.h"
#include "IAudioPort.h"

class TBaseEffect
{
UNCOPYABLE(TBaseEffect)
    ;

public:
    TBaseEffect()
        : Mix(0.0f)
    {
    }
    virtual ~TBaseEffect()
    {
    }
    virtual void Process(TSampleBufferCollection&, TSampleBufferCollection&)
    {
    }

    virtual void SetMix(TFraction mix)
    {
        Mix = mix;
    }

protected:
    TFraction Mix;
};
