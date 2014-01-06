#include "TJackSynth.h"

#include <iostream>

TJackSynth::TJackSynth(TAudioPortCollection& inputPorts,
		       TAudioPortCollection& outputPorts,
		       TAudioPortCollection& intermediateOutPorts)
  : InputPorts(inputPorts),
    OutputPorts(outputPorts),
    IntOutPorts(intermediateOutPorts),
    Program(),
    SendMidi(0)
{
  Program.SetParameterChangedCallback([this](int unit, int param, int value)
				      { ParameterChangedCallback(unit, param, value); });
  Program.Patch0();
}

TJackSynth::~TJackSynth()
{
}

void TJackSynth::HandleMidi(std::vector<uint8_t> data)
{
  uint8_t channel = data[0] & 0x0f;
  if (data.size() == 0) {
    return;
  } else if ((data[0] & 0xf0) == 0x90) {
    if (data[2] > 0) {
      if (channel == 0) Program.NoteOn(data[1], data[2]);
    } else {
      if (channel == 0) Program.NoteOff(data[1], data[2]);
    }
  } else if ((data[0] & 0xf0) == 0x80) {
    if (channel == 0) Program.NoteOff(data[1], data[2]);
  } else if ((data[0] & 0xf0) == 0xe0) {
    // Pitch bend
    int bend = ((data[2] << 7) | (data[1])) - 0x2000;
    if (channel == 0) Program.SetPitchBend((float)bend/0x2000); // -1..1
  } else if ((data[0] & 0xf0) == 0xb0) {
    if (channel == 0) Program.SetController(data[1], data[2]);
  } else if (data[0] == 0xf0 && data[1] == 0x7f) {
    // Set parameter sysex
    int param = data[2];
    int unit = data[3];
    int value = short((data[4] << 9) | (data[5] << 2)) >> 2;
    Program.SetParameter(unit, static_cast<TParameter>(param), value);
  } else if (data[0] == 0xf0 && data[1] == 0x7e) {
    Program.DumpParameters();
  } else if ((data[0] & 0xf0) == 0xc0) {
    // Program change
    if (channel == 0) {
      printf("Program change: %d\n", data[1]);
      if (data[1] == 0x00) {
	Program.Patch0();
      } else if (data[1] == 0x01) {
	Program.Patch1();
      } else if (data[1] == 0x02) {
	Program.Patch2();
      } else if (data[1] == 0x03) {
        Program.Patch3();
      }
      Program.DumpParameters();
    }
  } else {
    //printf("MIDI: %02x %02x %02x %02x\n", data[0], data[1], data[2], data[3]);
  }
}

void TJackSynth::ParameterChangedCallback(int unit, int param, int value)
{
  SendMidi({0xf0, 0x7f,
	uint8_t(param), uint8_t(unit),
	uint8_t((value >> 7) & 0x7f), uint8_t(value & 0x7f),
	0xf7});
}

void TJackSynth::SetPitchBend(float bend)
{
  Program.SetPitchBend(bend);
}

void TJackSynth::SetMidiSendCallback(std::function<void(const std::vector<uint8_t>&)> cb)
{
  SendMidi = cb;
}

int TJackSynth::Process(jack_nframes_t nframes)
{
  TSampleBuffer in[2] = { InputPorts[0]->GetBuffer(nframes), InputPorts[1]->GetBuffer(nframes) };
  TSampleBuffer out[2] = { OutputPorts[0]->GetBuffer(nframes), OutputPorts[1]->GetBuffer(nframes) };
  TSampleBuffer into[4] = { IntOutPorts[0]->GetBuffer(nframes), IntOutPorts[1]->GetBuffer(nframes),
      IntOutPorts[2]->GetBuffer(nframes), IntOutPorts[3]->GetBuffer(nframes)};

  TSampleBufferCollection ins({&in[0], &in[1]});
  TSampleBufferCollection outs({&out[0], &out[1]});
  TSampleBufferCollection ints({&into[0], &into[1], &into[2], &into[3]});

  out[0].Clear();
  out[1].Clear();
  into[0].Clear();
  into[1].Clear();
  into[2].Clear();
  into[3].Clear();

  Program.Process(ins, outs, ints);

  OutputPorts[0]->Commit();
  OutputPorts[1]->Commit();
  IntOutPorts[0]->Commit();
  IntOutPorts[1]->Commit();
  IntOutPorts[2]->Commit();
  IntOutPorts[3]->Commit();

  return 0;
}
