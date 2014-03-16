/* -*- mode: c++ -*- */
#pragma once

#include <string>
#include <algorithm>
#include <jack/jack.h>
#include "IAudioPort.h"
#include "util.h"

class TFileAudioPort: public IAudioPort
{
UNCOPYABLE(TFileAudioPort)
    ;

public:
    TFileAudioPort(std::string filename, TDirection direction);
    virtual TSampleBuffer GetBuffer(jack_nframes_t nframes);
    virtual void Commit();

private:
    TDirection Direction;
    FILE* Fd;
    TSampleBuffer* Buffer;
};
