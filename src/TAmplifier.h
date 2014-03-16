/* -*- mode: c++ -*- */
#pragma once

#include "IAudioPort.h"
#include <algorithm>

class TAmplifier
{
public:
    TAmplifier(TFraction volume = 1.0f)
            : Volume(volume)
    {
    }

    void SetVolume(TFraction vol)
    {
        Volume = vol;
    }
    void Process(TSampleBuffer& in, TSampleBuffer& out) const
    {
        std::transform(in.begin(), in.end(), out.begin(),
                [this](TSample s) -> TSample {
                    return Volume * s;
                });
    }

    TFraction Volume;
};
