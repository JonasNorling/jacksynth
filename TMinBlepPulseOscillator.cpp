/*
 * Measurements at 880Hz PW=50%, sample rate 44100Hz.
 *   --> 50.1 samples per period.
 * nzc=16, os=32 (--> 32 samples blep length):
 *    fundamental at -34dB, harmonics at -44 -48 -51 -53 -54 -57
 *    worst aliasing at -98dB, first peak at 538Hz.
 * nzc= 8, os=32 (--> 16 samples blep length):
 *    fundamental at -34dB
 *    worst aliasing at -98dB, first peak at 538Hz.
 * nzc=16, os=16 (--> 32 samples blep length):
 *    fundamental at -34dB
 *    worst aliasing at -91dB, first peak at 710Hz.
 * nzc= 8, os=64 (--> 16 samples blep length):
 *    fundamental at -34dB
 *    worst aliasing at -103dB, first peak at 237Hz.
 *    Double peaks above 16.5kHz
 * nzc=16, os=64 (--> 32 samples blep length):
 *    fundamental at -34dB
 *    worst aliasing at -104dB, first peak at 237Hz.
 *    Double peaks above 19kHz
 * nzc= 8, os=128 (--> 16 samples blep length):
 *    fundamental at -34dB
 *    horrible double peaks above 15kHz
 */

#include "TMinBlepPulseOscillator.h"
#include "TGlobal.h"

TMinBlepPulseOscillator::TMinBlepPulseOscillator() :
  Hz(0), Scanpos(0.99999999), BufferPos(0), Target(0), Pw(0), NextPw(0)
{
  std::fill(Buffer, Buffer+BufferLen, 0);
}

void TMinBlepPulseOscillator::Process(TSampleBuffer& in, TSampleBuffer& out)
{
  const float A = TGlobal::OscAmplitude;
  for (TSample& outs : out) {
    float pace = Hz/TGlobal::SampleRate;
    assert(pace > 0);
    if (pace > 0.99) pace = 0.99;

    double lastpos = Scanpos;
    Scanpos += pace;
    if (Scanpos >= 1) {
      Scanpos -= 1;
      Pw = NextPw;
    }
    const float pw = 0.5 + 0.5*Pw;
    assert(Scanpos >= 0);
    assert(Scanpos <= 1);

    // If Scanpos just went past 0, we need to place a positive
    // edge. If it just went past pw, we need to place a negative
    // edge. The step is constructed from minblep::length samples
    // chosen from minblep::table.

    if ((lastpos < pw && Scanpos >= pw) ||
	(lastpos < pw && lastpos > Scanpos)) {
      int offset = (Scanpos-pw)/pace * minblep::overSampling;
      if (Scanpos < pw) {
	offset = ((1+Scanpos)-pw)/pace * minblep::overSampling;
      }
      for (int i=0; i<BufferLen; i++) {
	Buffer[(BufferPos+i) % BufferLen] -= minblep::table[i*minblep::overSampling+offset];
      }
      Target = 0;
    }
    if (lastpos > Scanpos) {
      const int offset = Scanpos/pace * minblep::overSampling;
      for (int i=0; i<BufferLen; i++) {
	Buffer[(BufferPos+i) % BufferLen] += minblep::table[i*minblep::overSampling+offset];
      }
      Target = 1;
    }

    outs += A * (Buffer[BufferPos % BufferLen] - pw);
    Buffer[BufferPos % BufferLen] = Target;
    BufferPos++;
  }
}
