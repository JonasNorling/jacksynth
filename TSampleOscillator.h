/* -*- mode: c++ -*- */
#pragma once

#include <memory>
#include "IOscillator.h"
#include "TGlobal.h"

class TSampleOscillator : public IOscillator
{
  UNCOPYABLE(TSampleOscillator);

public:
  TSampleOscillator() :
    Hz(0), Scanpos(0), Sample()
  { }
  void SetFrequency(TFrequency hz) { Hz = hz; }
  void SetPulseWidth(float) { }
  void SetSample(TSampleBuffer* sample) { Sample = sample; }

  void Process(TSampleBuffer& in, TSampleBuffer& out);

private:
  double Hz;
  double Scanpos;
  TSampleBuffer* Sample;
};
