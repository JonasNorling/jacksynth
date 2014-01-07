/* -*- mode: c++ -*- */
#pragma once

#include "util.h"
#include "TGlobal.h"

class TPan
{
UNCOPYABLE(TPan)
    ;

public:
    TPan()
            : Left(0.5), Right(0.5)
    {
    }

    void Process(TSampleBuffer& in, TSampleBuffer& outl, TSampleBuffer& outr)
    {
        TSample* outli = outl.begin();
        TSample* outri = outr.begin();
        for (TSample* ini = in.begin(); ini != in.end();
                ini++, outli++, outri++) {
            *outli += Left * *ini;
            *outri += Right * *ini;
        }
    }

    void SetPan(float a, float v)
    {
        Right = a * (0.5 + v * 0.5);
        Left = a * (0.5 - v * 0.5);
    }

private:
    float Left;
    float Right;
};
