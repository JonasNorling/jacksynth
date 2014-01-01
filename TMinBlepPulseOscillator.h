/* -*- mode: c++ -*- */
#pragma once

#include "IAudioPort.h"
#include "IOscillator.h"
#include "minblep.h"
#include "util.h"

class TMinBlepPulseOscillator : public IOscillator
{
  UNCOPYABLE(TMinBlepPulseOscillator);

public:
  TMinBlepPulseOscillator();

  void SetFrequency(TFrequency hz) { Hz = hz; }
  /* pw = 0 --> 50% duty cycle
   * pw = 1 --> 100% duty cycle */
  void SetPulseWidth(float pw) { NextPw = clamp(pw, 0.0f, 0.95f); }
  void SetSync(bool sync) { }

  void Process(TSampleBuffer& in, TSampleBuffer& out, TSampleBuffer& syncin, TSampleBuffer& syncout);

private:
  TFrequency Hz;
  double Scanpos;
  static const int BufferLen = minblep::length;
  TSample Buffer[BufferLen];
  int BufferPos;
  int Target;
  float Pw;
  float NextPw;
};
