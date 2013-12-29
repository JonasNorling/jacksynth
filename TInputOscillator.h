/* -*- mode: c++ -*- */
#pragma once

#include <cmath>
#include "IOscillator.h"
#include "TGlobal.h"

class TInputOscillator : public IOscillator
{
public:
  TInputOscillator()
  {
  }
  void SetFrequency(TFrequency) { }
  void SetPulseWidth(float) { }

  void Process(TSampleBuffer& in, TSampleBuffer& out)
  {
    out.AddSamples(in);
  }
};
