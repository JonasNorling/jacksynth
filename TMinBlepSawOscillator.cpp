#include "TMinBlepSawOscillator.h"
#include "TGlobal.h"

TMinBlepSawOscillator::TMinBlepSawOscillator() :
  Hz(0), Scanpos(0.99999999), BufferPos(0), Target(0)
{
  std::fill(Buffer, Buffer+BufferLen, 0);
}

/*
 *  |    |    |
 *  |   /|   /|
 *  |  / |  / |
 *  | /  | /  | /
 *  |/   |/   |/
 *
 *  ^    ^    ^  Place negative minBLEPs here.
 */
void TMinBlepSawOscillator::Process(TSampleBuffer& in, TSampleBuffer& out)
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
    }
    assert(Scanpos >= 0);
    assert(Scanpos <= 1);

    if (lastpos > Scanpos) { // Wrapped around to beginning of wave
      const int offset = Scanpos/pace * minblep::overSampling;
      for (int i=0; i<BufferLen; i++) {
	Buffer[(BufferPos+i) % BufferLen] += -1 + minblep::table[i*minblep::overSampling+offset];
      }
    }

    outs += A * (0.5 + Scanpos - Buffer[BufferPos % BufferLen]);
    Buffer[BufferPos % BufferLen] = 1;
    BufferPos++;
  }
}
