#include "TMinBlepSawOscillator.h"
#include "TGlobal.h"

TMinBlepSawOscillator::TMinBlepSawOscillator() :
  Hz(0), Sync(false), Scanpos(0.99999999), BufferPos(0), Target(0)
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
void TMinBlepSawOscillator::Process(TSampleBuffer& in, TSampleBuffer& out, TSampleBuffer& syncin, TSampleBuffer& syncout)
{
  const float A = TGlobal::OscAmplitude;
  for (size_t sn = 0; sn < out.GetCount(); sn++) {
    float pace = Hz/TGlobal::SampleRate;
    assert(pace > 0);
    if (pace > 0.99) pace = 0.99;

    double lastpos = Scanpos;
    Scanpos += pace;
    if (Scanpos >= 1) {
      Scanpos -= 1;
    }

    float stepA = 1.0f;
    if (Sync && syncin[sn] > 0) {
      stepA = lastpos; // FIXME: This is not precise
      Scanpos = syncin[sn] * pace; // FIXME: Is this right?
    }

    assert(Scanpos >= 0);
    assert(Scanpos <= 1);

    if (lastpos > Scanpos) {
      // Wrapped around or synced to beginning of wave. Place a step here.
      syncout[sn] = Scanpos / pace + 0.0000001;
      const int offset = Scanpos/pace * minblep::overSampling;
      for (int i=0; i<BufferLen; i++) {
	Buffer[(BufferPos+i) % BufferLen] += stepA * (-1 + minblep::table[i*minblep::overSampling+offset]);
      }
    }
    else {
      syncout[sn] = 0;
    }

    // Correct for the fact that the BLEP has an average (integral) that is different from a perfect step
    TSample offsetCorrection = (minblep::integral - minblep::length) * pace;

    out[sn] += A * (0.5 + Scanpos - Buffer[BufferPos % BufferLen] + offsetCorrection);
    Buffer[BufferPos % BufferLen] = 1;
    BufferPos++;
  }
}
