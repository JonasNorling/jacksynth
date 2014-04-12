#include "TJackAudioPort.h"

TJackAudioPort::TJackAudioPort(std::string name, jack_client_t* client,
        TDirection direction)
        : IAudioPort(), Client(client), Direction(direction), Port(0)
{
    Port = jack_port_register(Client, name.c_str(),
            JACK_DEFAULT_AUDIO_TYPE, Direction, 0);
    assert(Port);
}

TSampleBuffer TJackAudioPort::GetBuffer(jack_nframes_t nframes)
{
    return TSampleBuffer(
            static_cast<jack_default_audio_sample_t*>(jack_port_get_buffer(Port,
                    nframes)), nframes);
}

bool TJackAudioPort::Connect(const std::string& port)
{
    const char* me = jack_port_name(Port);
    if (Direction == INPUT) {
        return jack_connect(Client, port.c_str(), me) == 0;
    }
    else {
        return jack_connect(Client, me, port.c_str()) == 0;
    }
}
