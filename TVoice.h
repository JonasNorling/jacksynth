/* -*- mode: c++ -*- */
#pragma once

#include "TBaseOscillator.h"
#include "TButterworthLpFilter.h"
#include "TResonantLpFilter.h"
#include "TSvfFilter.h"
#include "TEnvelope.h"
#include "TAmplifier.h"
#include "TLfo.h"
#include "TWaveShaper.h"
#include "TPan.h"
#include "util.h"
#include "TGlobal.h"

class TVoice
{
UNCOPYABLE(TVoice)
    ;

public:
    TVoice(TFrequency hz, TUnsigned7 velocity)
            : State(PLAYING), Hold(false), Hz(hz), Velocity(velocity), AmpEg(), FiltEg()
    {
    }

    ~TVoice()
    {
    }

    enum TState
    {
        PLAYING,
        SUSTAINED, // waiting for sustain pedal to be released
        RELEASED, // key released; envelope in release phase
        FINISHED, // no longer playing, please delete me
    } State;

    bool Hold; // Note is held by sostenuto pedal

    TFrequency Hz;
    TUnsigned7 Velocity;

    std::unique_ptr<TBaseOscillator> Oscillators[TGlobal::Oscillators];
    TPan OscPan[TGlobal::Oscillators];
    TWaveShaper WaveShaper[TGlobal::Filters];
    TSvfFilter Filters[TGlobal::Filters];
    TPan FiltPan[TGlobal::Filters];
    TLfo Lfos[TGlobal::Lfos];
    TEnvelope AmpEg;
    TEnvelope FiltEg;
};
