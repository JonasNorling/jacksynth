/* -*- mode: c++ -*- */
#pragma once

#include "IEffect.h"

class TDelayFx : public IEffect
{
  UNCOPYABLE(TDelayFx);
public:
  TDelayFx();
  virtual void Process(TSampleBufferCollection& in, TSampleBufferCollection& out);

private:
  static const size_t BufferSize = 48000;
  TSample Buffer[2][BufferSize];
  int Delay; // in samples
  int ReadPos;
};

