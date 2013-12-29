/* -*- mode: c++ -*- */
#pragma once

#include <cmath>
#include "IOscillator.h"
#include "TGlobal.h"

class TSineOscillator : public IOscillator
{
public:
  TSineOscillator() : Hz(0), Scanpos(0)
  {
  }
  void SetFrequency(TFrequency hz) { Hz = hz; }
  void SetPulseWidth(float) { }

  void Process(TSampleBuffer& in, TSampleBuffer& out)
  {
    // FIXME: This is quite slow for some reason. Also, don't
    // increment scanpos for ever, it's going to lose precision.
    for (TSample& outs : out) {
      outs = TGlobal::OscAmplitude * sinf(Scanpos);
      Scanpos += Hz/TGlobal::SampleRate * 2*M_PI;
    }
  }

private:
  double Hz;
  double Scanpos;
};
