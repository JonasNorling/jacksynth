/* -*- mode: c++ -*- */
#pragma once

#include <string>
#include <algorithm>
#include <jack/jack.h>
#include "IAudioPort.h"
#include "util.h"

class TJackAudioPort: public IAudioPort
{
UNCOPYABLE(TJackAudioPort)
    ;

public:
    TJackAudioPort(std::string name, jack_client_t* client, TDirection direction);
    virtual TSampleBuffer GetBuffer(jack_nframes_t nframes);
    virtual void Commit() { }
    bool Connect(const std::string& port);

private:
    jack_client_t* Client;
    TDirection Direction;
    jack_port_t* Port;
};
