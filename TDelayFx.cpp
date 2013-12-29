#include "TDelayFx.h"
#include "TGlobal.h"

TDelayFx::TDelayFx() :
  Delay(ms_to_samples(500)),
  ReadPos(0)
{
  std::fill(Buffer[0], Buffer[0]+BufferSize, 0);
  std::fill(Buffer[1], Buffer[1]+BufferSize, 0);
}

void TDelayFx::Process(TSampleBufferCollection& in, TSampleBufferCollection& out)
{
  // assert stereo...
  assert(in.size() == 2);
  assert(out.size() == 2);

  // FIXME: Mix, pingpongishness, lowpass.

  const float pingpong = 0.8; /* 0.5 is neutral and will cause a mono-downmix in the buffer. */
  const float feedback = 0.75;
  const int framesize = in[0]->GetCount();

  for (int i = 0; i < framesize; i++) {
    const int writePos = (ReadPos + Delay) % BufferSize;

    Buffer[0][writePos] = feedback * (pingpong * (*in[0])[i] +
				      (1-pingpong) * Buffer[0][ReadPos] +
				      pingpong * Buffer[1][ReadPos]);
    (*out[0])[i] = (*in[0])[i] + Buffer[0][ReadPos];

    Buffer[1][writePos] = feedback * ((1-pingpong) * (*in[1])[i] +
				      (1-pingpong) * Buffer[1][ReadPos] +
				      pingpong * Buffer[0][ReadPos]);
    (*out[1])[i] = (*in[1])[i] + Buffer[1][ReadPos];

    ReadPos = (ReadPos + 1) % BufferSize;		     
  }
}
