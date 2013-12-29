/* -*- mode: c++ -*- */
#pragma once

#include "IAudioPort.h"
#include <algorithm>

class TAmplifier
{
public:
  TAmplifier() :
    Volume(1)
  { }

  void SetVolume(TFraction vol) { Volume = vol; }
  void Process(TSampleBuffer& in, TSampleBuffer& out) const
  {
    std::transform(in.begin(), in.end(),
		   out.begin(),
		   [Volume](TSample s) -> TSample {
		     return Volume * s;
		   });
  }

  TFraction Volume;
};
