/* -*- mode: c++ -*- */
#pragma once

#include "IAudioPort.h"
#include "IOscillator.h"
#include "minblep.h"
#include "util.h"

class TMinBlepSawOscillator : public IOscillator
{
public:
  TMinBlepSawOscillator();

  void SetFrequency(TFrequency hz) { Hz = hz; }
  void SetPulseWidth(float) { }
  void Process(TSampleBuffer& in, TSampleBuffer& out);

private:
  TFrequency Hz;
  double Scanpos;
  static const int BufferLen = minblep::length;
  TSample Buffer[BufferLen];
  int BufferPos;
  TSample Target;
};
