#include "TDelayFx.h"
#include "TGlobal.h"

TDelayFx::TDelayFx()
        : TBaseEffect(), Delay(ms_to_samples(500)), Feedback(0.75), WritePos(0), Distortion(0.0f),
          LfoDepth(ms_to_samples(0)), LfoSpeed(0.0f), LfoChannelPhase(0.6f), LfoPhaseAccumulator(0.0f),
          InsertGain(1.0f), Pingpong(0.8f)
{
    std::fill(Buffer[0], Buffer[0] + BufferSize, 0);
    std::fill(Buffer[1], Buffer[1] + BufferSize, 0);
    Filter[0].SetCutoffHz(3000);
    Filter[1].SetCutoffHz(3000);
}

void TDelayFx::Process(TSampleBufferCollection& in, TSampleBufferCollection& out)
{
    // assert stereo...
    assert(in.size() == 2);
    assert(out.size() == 2);

    const int framesize = in[0]->GetCount();

    TSampleBuffer& inl = *in[0];
    TSampleBuffer& inr = *in[1];
    TSampleBuffer& outl = *out[0];
    TSampleBuffer& outr = *out[1];

    // Write delayed samples in the buffer, just like a tape delay.
    // The buffer size does not really matter; the Delay parameter sets the delay in effect.
    // The buffer size just has to be larger than the delay in samples.
    for (int i = 0; i < framesize; i++) {
        LfoPhaseAccumulator += LfoSpeed;
        const float delayl = Delay + LfoDepth * sinf(LfoPhaseAccumulator);
        const float delayr = Delay + LfoDepth * sinf(LfoPhaseAccumulator + LfoChannelPhase * M_PI);

        // Pull out interpolated values from the buffer, based on the LFO modulated offset.
        // We add BufferSize to the position so we get a positive number to do modulo with.
        const TSample samplel = linterpolate(Buffer[0], BufferSize, BufferSize + WritePos - delayl);
        const TSample sampler = linterpolate(Buffer[1], BufferSize, BufferSize + WritePos - delayr);

        TSample vl = InsertGain * (Pingpong * inl[i]) +
                Feedback * ((1 - Pingpong) * samplel + Pingpong * sampler);

        TSample vr = InsertGain * ((1 - Pingpong) * inr[i]) +
                Feedback * ((1 - Pingpong) * sampler + Pingpong * samplel);

        Buffer[0][WritePos] = Distortion.MusicDsp1(Filter[0].ProcessOne(vl));
        Buffer[1][WritePos] = Distortion.MusicDsp1(Filter[1].ProcessOne(vr));

        outl[i] = mix(inl[i], samplel, Mix);
        outr[i] = mix(inr[i], sampler, Mix);

        WritePos = (WritePos + 1) % BufferSize;
    }
}
