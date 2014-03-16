/* -*- mode: c++ -*- */
#pragma once

#include "filters.h"
#include "TBaseEffect.h"

class TReverbFx: public TBaseEffect
{
UNCOPYABLE(TReverbFx)
    ;
public:
    TReverbFx();
    virtual void Process(TSampleBufferCollection& in, TSampleBufferCollection& out);

private:
    static const size_t BufferSize = 0x10000;
    TSample Buffer[2][BufferSize];
    int ReadPos;
    TDcBlocker DcBlocker[2];
    TOnePoleLpFilter LpFilter[2];
};
