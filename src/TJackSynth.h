/* -*- mode: c++ -*- */
#pragma once

#include <jack/jack.h>
#include <jack/midiport.h>
#include <string>
#include <memory>
#include <map>

#include "TClockRecovery.h"
#include "TJackAudioPort.h"
#include "TProgram.h"

/**
 * Hold the active program (patch) for a MIDI channel. When the program is changed
 * it is kept around until it goes quiet, as a zombie program.
 */
class TChannel
{
    UNCOPYABLE(TChannel)
        ;

public:
    TChannel();
    void ChangeProgram(std::unique_ptr<TProgram>& p);
    void NoteOn(TUnsigned7 note, TUnsigned7 velocity);
    void NoteOff(TUnsigned7 note, TUnsigned7 velocity);

    std::unique_ptr<TProgram> ActiveProgram;
    std::list< std::unique_ptr<TProgram> > ZombiePrograms;
    bool Active; ///< Listening on this MIDI channel
};

class TJackSynth
{
    UNCOPYABLE(TJackSynth)
        ;

public:
    TJackSynth(TAudioPortCollection& inputPorts,
            TAudioPortCollection& outputPorts,
            TAudioPortCollection& intermediateOutPorts);

    virtual ~TJackSynth();
    int Process(jack_nframes_t nframes);
    void HandleMidi(std::vector<uint8_t> data, uint64_t timestamp = 0);
    void SetPitchBend(float bend); ///< Debugging crap
    void SetMidiSendCallback(std::function<void(const std::vector<uint8_t>&)> cb);

private:
    TTimer TimerProcess;

    TAudioPortCollection& InputPorts;
    TAudioPortCollection& OutputPorts;
    TAudioPortCollection& IntOutPorts;

    TClockRecovery ClockRecovery;

    std::vector<TChannel> Channels;

    void ParameterChangedCallback(int unit, int param, int value);
    std::function<void(const std::vector<uint8_t>&)> SendMidi;

    void HandleWaldorfSysex(TProgram& program, std::vector<uint8_t> data);
};
