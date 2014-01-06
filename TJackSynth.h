/* -*- mode: c++ -*- */
#pragma once

#include <jack/jack.h>
#include <jack/midiport.h>
#include <string>
#include <memory>
#include <map>

#include "TJackAudioPort.h"
#include "TProgram.h"

class TJackSynth
{
public:
  TJackSynth(TAudioPortCollection& inputPorts,
	     TAudioPortCollection& outputPorts,
	     TAudioPortCollection& intermediateOutPorts);

  virtual ~TJackSynth();
  int Process(jack_nframes_t nframes);
  void HandleMidi(std::vector<uint8_t> data);
  void SetPitchBend(float bend); ///< Debugging crap
  void SetMidiSendCallback(std::function<void(const std::vector<uint8_t>&)> cb);

private:
  TAudioPortCollection& InputPorts;
  TAudioPortCollection& OutputPorts;
  TAudioPortCollection& IntOutPorts;

  // Just one program for now, we should go multi-timbral in the future
  TProgram Program;

  void ParameterChangedCallback(int unit, int param, int value);
  std::function<void(const std::vector<uint8_t>&)> SendMidi;

  void HandleWaldorfSysex(std::vector<uint8_t> data);
};
