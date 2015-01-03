#include "TReverbFx.h"
#include "TGlobal.h"

TReverbFx::TReverbFx()
        : TBaseEffect(), ReadPos(0)
{
    std::fill(Buffer[0], Buffer[0] + BufferSize, 0);
    std::fill(Buffer[1], Buffer[1] + BufferSize, 0);
    LpFilter[0].SetCutoff(0.02);
    LpFilter[1].SetCutoff(0.02);
}

void TReverbFx::Process(TSampleBufferCollection& in, TSampleBufferCollection& out)
{
    // assert stereo...
    assert(in.size() == 2);
    assert(out.size() == 2);

    const unsigned framesize = in[0]->GetCount();

    TSampleBuffer& inl = *in[0];
    TSampleBuffer& inr = *in[1];
    TSampleBuffer& outl = *out[0];
    TSampleBuffer& outr = *out[1];

    const unsigned maxDelay = 24000;
    static_assert(maxDelay < BufferSize, "fail");

    TSample samefeedback = 0.3;
    TSample otherfeedback = 0.2;

    for (unsigned i = 0; i < framesize; i++) {
        Buffer[0][(ReadPos + maxDelay) % BufferSize] = 0.0f;
        Buffer[1][(ReadPos + maxDelay) % BufferSize] = 0.0f;

        TSample vl = DcBlocker[0].ProcessOne(LpFilter[0].ProcessOne(
                inl[i] + samefeedback * Buffer[0][ReadPos] + otherfeedback * Buffer[1][ReadPos]));
        TSample vr = DcBlocker[1].ProcessOne(LpFilter[1].ProcessOne(
                inr[i] + samefeedback * Buffer[1][ReadPos] + otherfeedback * Buffer[0][ReadPos]));

        unsigned d = 137;
        while (d < maxDelay) {
            TSample fl = ((d * 4711) % 1337) * (1.0f - (float(d) / maxDelay));
            TSample fr = ((d * 6347) % 1337) * (1.0f - (float(d) / maxDelay));
            Buffer[0][(ReadPos + d) % BufferSize] += vl * fl / 6000.0f;
            Buffer[1][(ReadPos + d) % BufferSize] += vr * fr / 6000.0f;
            d += ((d * 5684) % 397);
        }

        outl[i] = mix(inl[i], Buffer[0][ReadPos], Mix);
        outr[i] = mix(inr[i], Buffer[1][ReadPos], Mix);

        ReadPos = (ReadPos + 1) % BufferSize;
    }
}
