#include <cassert>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdio>
#include "TFileAudioPort.h"

TFileAudioPort::TFileAudioPort(std::string filename, TDirection direction)
        : Direction(direction), Fd(0), Buffer(0)
{
    if (direction == OUTPUT) {
        Fd = fopen(filename.c_str(), "w");
        assert(Fd);
    }
}

TSampleBuffer TFileAudioPort::GetBuffer(jack_nframes_t nframes)
{
    if (Buffer) {
        delete[] Buffer->begin();
    }
    Buffer = new TSampleBuffer(new TSample[nframes], nframes);
    Buffer->Clear();
    return *Buffer;
}

void TFileAudioPort::Commit()
{
    if (Direction == OUTPUT) {
        for (TSample& s : *Buffer) {
            //fprintf(Fd, "%f\n", s);
            float sample = s;
            fwrite(&sample, sizeof(float), 1, Fd);
        }
    }
}
