#include <string>
#include <ctime>

#include "TEnvelope.h"
#include "TGlobal.h"

TFraction TEnvelope::Step(int samples)
{
    SampleCounter += samples;

    switch (State) {
    case IDLE:
        SampleCounter = 0;
        Value = 0;
        State = ATTACK;
        // Fall-through
    case ATTACK:
        if (SampleCounter < A) {
            Value = float(SampleCounter) / A;
        }
        else {
            SampleCounter = 0;
            Value = 1;
            State = DECAY;
        }
        break;
    case DECAY:
        if (SampleCounter < D) {
            Value = 1 - (1 - S) * float(SampleCounter) / D;
        }
        else {
            SampleCounter = 0;
            Value = S;
            State = SUSTAIN;
        }
        break;
    case SUSTAIN:
        break;
    case RELEASE:
        if (SampleCounter < R) {
            Value = ReleaseValue - ReleaseValue * float(SampleCounter) / R;
        }
        else {
            Value = 0;
            State = FINISHED;
        }
        break;
    case FINISHED:
        break;
    }
    return Value;
}

void TEnvelope::Release(TUnsigned7 velocity)
{
    SampleCounter = 0;
    ReleaseValue = Value;
    State = RELEASE;
}
