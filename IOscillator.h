/* -*- mode: c++ -*- */
#pragma once

#include "util.h"
#include "IAudioPort.h"

class IOscillator
{
  UNCOPYABLE(IOscillator);

public:
  IOscillator() {}
  virtual ~IOscillator() {}

  virtual void SetFrequency(TFrequency hz) = 0;
  virtual void SetPulseWidth(float pw) = 0;
  virtual void Process(TSampleBuffer& in, TSampleBuffer& out) = 0;
};

class TNoOscillator : public IOscillator
{
public:
  TNoOscillator() {}
  virtual ~TNoOscillator() {}

  void SetFrequency(TFrequency) { };
  void SetPulseWidth(float) { };
  void Process(TSampleBuffer&, TSampleBuffer&) { }
};
