#include "TDelayFx.h"
#include "TGlobal.h"

TDelayFx::TDelayFx()
        : TBaseEffect(), Delay(ms_to_samples(500)), Feedback(0.75), ReadPos(0)
{
    std::fill(Buffer[0], Buffer[0] + BufferSize, 0);
    std::fill(Buffer[1], Buffer[1] + BufferSize, 0);
}

void TDelayFx::Process(TSampleBufferCollection& in, TSampleBufferCollection& out)
{
    // assert stereo...
    assert(in.size() == 2);
    assert(out.size() == 2);

    const float pingpong = 0.8; /* 0.5 is neutral and will cause a mono-downmix in the buffer. */
    const int framesize = in[0]->GetCount();

    TSampleBuffer& inl = *in[0];
    TSampleBuffer& inr = *in[1];
    TSampleBuffer& outl = *out[0];
    TSampleBuffer& outr = *out[1];

    // Write delayed samples in the buffer, just like a tape delay.
    // The buffer size does not really matter; the Delay parameter sets the delay in effect.
    // The buffer size just has to be larger than the delay in samples.
    for (int i = 0; i < framesize; i++) {
        const int writePos = (ReadPos + Delay) % BufferSize;

        TSample vl = Feedback * (pingpong * inl[i] +
                (1 - pingpong) * Buffer[0][ReadPos] +
                pingpong * Buffer[1][ReadPos]);

        TSample vr = Feedback * ((1 - pingpong) * inr[i] +
                (1 - pingpong) * Buffer[1][ReadPos] +
                pingpong * Buffer[0][ReadPos]);

        Buffer[0][writePos] = vl;
        Buffer[1][writePos] = vr;

        outl[i] = mix(inl[i], Buffer[0][ReadPos], Mix);
        outr[i] = mix(inr[i], Buffer[1][ReadPos], Mix);

        ReadPos = (ReadPos + 1) % BufferSize;
    }
}
