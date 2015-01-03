#pragma once

#include <sndfile.h>
#include <memory>
#include "IAudioPort.h"
#include "util.h"

class TAudioFileWriter
{
UNCOPYABLE(TAudioFileWriter)
    ;

public:
    class TAudioPort : public IAudioPort {
    public:
        TAudioPort(const TAudioPort& p);
        TAudioPort(TAudioFileWriter& writer, unsigned channel);
        TSampleBuffer GetBuffer(jack_nframes_t nframes);
        void Commit();

    private:
        TAudioFileWriter& Writer;
        std::vector<TSample> Buffer;
        unsigned Channel;
    };

    TAudioFileWriter(std::string filename, unsigned channels, unsigned samplerate);
    ~TAudioFileWriter();
    TAudioPortCollection GetPorts();

private:
    void Write(const TSampleBuffer& data, unsigned channel);

    unsigned Channels;
    std::vector<std::unique_ptr<TAudioPort>> Ports;
    std::vector<bool> ChannelWritten;
    SNDFILE* Handle;
    std::vector<TSample> Buffer;
};
