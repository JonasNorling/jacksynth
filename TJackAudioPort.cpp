#include "TJackAudioPort.h"

TJackAudioPort::TJackAudioPort(std::string name, jack_client_t* client,
        TDirection direction)
        : IAudioPort(), Port(0)
{
    Port = jack_port_register(client, name.c_str(),
    JACK_DEFAULT_AUDIO_TYPE, direction, 0);
    assert(Port);
}

TSampleBuffer TJackAudioPort::GetBuffer(jack_nframes_t nframes)
{
    return TSampleBuffer(
            static_cast<jack_default_audio_sample_t*>(jack_port_get_buffer(Port,
                    nframes)), nframes);
}
