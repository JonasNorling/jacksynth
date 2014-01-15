/* -*- mode: c++ -*- */
#pragma once

#include "IAudioPort.h"
#include "TBaseOscillator.h"
#include "minblep.h"
#include "util.h"

class TMinBlepPulseOscillator: public TBaseOscillator
{
UNCOPYABLE(TMinBlepPulseOscillator)
    ;

public:
    TMinBlepPulseOscillator(const TNoteData& noteData);

    /* pw = 0 --> 50% duty cycle
     * pw = 1 --> 100% duty cycle */
    void SetPulseWidth(float pw)
    {
        NextPw = clamp(pw, 0.0f, 0.95f);
    }

    void Process(TSampleBuffer& in, TSampleBuffer& out, TSampleBuffer& syncin,
            TSampleBuffer& syncout);

private:
    static const int BufferLen = minblep::length;
    TSample Buffer[BufferLen];
    int BufferPos;
    int Target;
    float Pw;
    float NextPw;
};
