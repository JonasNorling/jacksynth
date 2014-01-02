/* -*- mode: c++ -*- */
#pragma once

#include "IOscillator.h"
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
  UNCOPYABLE(TVoice);

public:
  TVoice(TFrequency hz, TUnsigned7 velocity) :
    State(PLAYING),
    Hz(hz),
    Velocity(velocity),
    DeleteMe(false),
    AmpEg(),
    FiltEg()
  { }

  ~TVoice()
  { }

  enum TState {
    PLAYING,
    SUSTAINED, // waiting for sustain pedal to be released
    RELEASED
  } State;

  TFrequency Hz;
  TUnsigned7 Velocity;
  bool DeleteMe; //< Voice has finished playing

  std::unique_ptr<IOscillator> Oscillators[TGlobal::Oscillators];
  TPan OscPan[TGlobal::Oscillators];
  TWaveShaper WaveShaper[TGlobal::Filters];
  TSvfFilter Filters[TGlobal::Filters];
  TPan FiltPan[TGlobal::Filters];
  TLfo Lfos[TGlobal::Lfos];
  TEnvelope AmpEg;
  TEnvelope FiltEg;
};
