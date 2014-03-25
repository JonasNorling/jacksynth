/* -*- mode: c++ -*- */
#pragma once

#include "util.h"
#include "TGlobal.h"

class TEnvelope
{
UNCOPYABLE(TEnvelope)
    ;

public:
    enum TState
    {
        IDLE, ATTACK, DECAY, SUSTAIN, RELEASE, FINISHED
    };

    struct TSettings
    {
        TTime Attack; // ms
        TTime Decay; // ms
        TFraction Sustain; // level
        TTime Release; // ms
    };

    TEnvelope()
            : State(IDLE), Value(0), SampleCounter(0), ReleaseValue(0), A(0), D(0), S(0), R(0)
    {
    }

    void Reset(const TSettings& s)
    {
        State = IDLE;
        SampleCounter = 0;
        ReleaseValue = 0;
        A = s.Attack * TGlobal::SampleRate / 1000.0;
        D = s.Decay * TGlobal::SampleRate / 1000.0;
        S = s.Sustain;
        R = s.Release * TGlobal::SampleRate / 1000.0;
    }
    TFraction Step(int samples);
    void Release(TUnsigned7 velocity);
    TState GetState() const
    {
        return State;
    }
    TFraction GetValue() const
    {
        return Value;
    }

private:
    TState State;
    TFraction Value;
    unsigned SampleCounter;
    TFraction ReleaseValue;

    unsigned A; // samples
    unsigned D; // samples
    TFraction S; // sustain level
    unsigned R; // samples
};
