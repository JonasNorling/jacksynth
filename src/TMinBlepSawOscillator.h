/* -*- mode: c++ -*- */
#pragma once

#include "IAudioPort.h"
#include "TBaseOscillator.h"
#include "minblep.h"
#include "util.h"

class TMinBlepSawOscillator: public TBaseOscillator
{
public:
    TMinBlepSawOscillator(const TNoteData& noteData);

    void Process(TSampleBuffer& in, TSampleBuffer& out, TSampleBuffer& syncin,
            TSampleBuffer& syncout);

private:
    static const int BufferLen = minblep::length;
    TSample Buffer[BufferLen];
    int BufferPos;
};
