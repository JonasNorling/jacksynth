#include <unistd.h>
#include <getopt.h>
#include <iostream>
#include <signal.h>
#include <queue>
#include "TJackSynth.h"
#include "TFileAudioPort.h"
#include "TGlobal.h"

int TGlobal::SampleRate;
int TGlobal::NyquistFrequency;

jack_client_t* Client;
jack_port_t* MidiIn;
jack_port_t* MidiOut;
std::queue< std::vector<uint8_t> > MidiOutQueue;

volatile bool die = false;
void sigterm(int)
{
  die = true;
}

int process(jack_nframes_t nframes, void* arg)
{
  TJackSynth* synth = static_cast<TJackSynth*>(arg);

  /* Process MIDI input */
  void* inbuf = jack_port_get_buffer(MidiIn, nframes);
  jack_nframes_t event_count = jack_midi_get_event_count(inbuf);
  for (jack_nframes_t i=0; i<event_count; i++) {
    jack_midi_event_t event;
    jack_midi_event_get(&event, inbuf, i);
    std::vector<uint8_t> data(event.buffer, event.buffer+event.size);
    synth->HandleMidi(data);
  }

  /* Send MIDI */
  void* outbuf = jack_port_get_buffer(MidiOut, nframes);
  jack_midi_clear_buffer(outbuf);
  while (!MidiOutQueue.empty()) {
    const std::vector<uint8_t>& data = MidiOutQueue.front();
    int ret = jack_midi_event_write(outbuf, 0,
				    data.data(), data.size());
    MidiOutQueue.pop();
    if (ret != 0) {
      fprintf(stderr, "MIDI send error\n");
    }
  }

  return synth->Process(nframes);
}

void send_midi(const std::vector<uint8_t>& data)
{
  MidiOutQueue.push(data);
}

void testSignalSawSweep(TJackSynth& synth)
{
  // This code depends on the default patch being a fairly
  // clean saw wave.

  synth.HandleMidi({0x90, 0, 0x40}); // note 0: 8.18 Hz
  synth.Process(int(0.5*44100) & ~0x7);
  synth.HandleMidi({0x80, 0, 0x40}); // release note

  // Sweep
  synth.HandleMidi({0x90, 33, 0x40}); // note 33: A 55Hz
  synth.Process(int(0.2*44100) & ~0x7);
  for (float octave = 0; octave < 6; octave += 0.001) {
    synth.SetPitchBend(octave * 6);
    synth.Process(16);
  }
  synth.Process(int(0.1*44100) & ~0x7);
  synth.HandleMidi({0x80, 33, 0x40}); // release note
  synth.Process(int(0.8*44100) & ~0x7);
}

void testSignalFilterSweep(TJackSynth& synth)
{
  int dist = 8000;
  synth.HandleMidi({0xf0, 0x7f, PARAM_DISTORTION, 1, hi7(dist), lo7(dist), 0xf7});
  synth.HandleMidi({0x90, TGlobal::MidiNoteA4, 0x70});
  synth.Process(int(0.5*44100) & ~0x7);

  for (float hz = 1; hz < 8000; hz++) {
    synth.HandleMidi({0xf0, 0x7f, PARAM_FILTER_CUTOFF_HZ, 0, hi7(hz), lo7(hz), 0xf7});
    synth.HandleMidi({0xf0, 0x7f, PARAM_FILTER_CUTOFF_HZ, 1, hi7(hz), lo7(hz), 0xf7});
    synth.Process(32);
  }

  synth.Process(int(0.5*44100) & ~0x7);
}

void runInJack()
{
  Client = jack_client_open("jacksynth", JackNullOption, 0);
  if (!Client) {
    std::cerr << "Cannot connect to JACK" << std::endl;
    abort();
  }

  std::cout << "Connected to JACK:" << std::endl
	    << "  Sample rate " << jack_get_sample_rate(Client) << " Hz" << std::endl
	    << "  Buffer size " << jack_get_buffer_size(Client) << " frames" << std::endl
	    << "  Sample size " << sizeof(TSample) << " bytes" << std::endl;

  TJackAudioPort inputPortL("leftin", Client, TJackAudioPort::INPUT);
  TJackAudioPort inputPortR("rightin", Client, TJackAudioPort::INPUT);
  TJackAudioPort outputPortL("left", Client, TJackAudioPort::OUTPUT);
  TJackAudioPort outputPortR("right", Client, TJackAudioPort::OUTPUT);

  MidiIn = jack_port_register(Client, "MIDI-IN",
			      JACK_DEFAULT_MIDI_TYPE,
			      JackPortIsInput, 0);

  MidiOut = jack_port_register(Client, "MIDI-OUT",
			       JACK_DEFAULT_MIDI_TYPE,
			       JackPortIsOutput, 0);

  TGlobal::SampleRate = jack_get_sample_rate(Client);
  TGlobal::NyquistFrequency = TGlobal::SampleRate / 2;

  TAudioPortCollection inputPorts({&inputPortL, &inputPortR});
  TAudioPortCollection outputPorts({&outputPortL, &outputPortR});
  TJackSynth synth(inputPorts, outputPorts);
  synth.SetMidiSendCallback(send_midi);

  jack_set_process_callback(Client, process, &synth);
  if (jack_activate(Client)) {
    std::cerr << "Cannot start jackiness" << std::endl;
    abort();
  }

  while (!die) sleep(1);
  jack_client_close(Client);
}

int main(int argc, char* argv[])
{
  int testsignal = 0;

  struct option longopts[] = {{ "testsignal", 1, 0, 't' },
                              { "help", 0, 0, 'h'},
                              { 0, 0, 0, 0}};
  int opt;
  while ((opt = getopt_long(argc, argv, "h", longopts, 0)) != -1) {
    switch (opt) {
    case 'h':
      std::cerr << "Help!" << std::endl;
      break;
    case 't':
      testsignal = atoi(optarg);
      break;
    default:
      std::cerr << "Bah!" << std::endl;
      return 1;
    }
  }

  if (testsignal) {
    TFileAudioPort inputPortL("", TFileAudioPort::INPUT);
    TFileAudioPort inputPortR("", TFileAudioPort::INPUT);
    TAudioPortCollection inputPorts({&inputPortL, &inputPortR});
    TFileAudioPort outputPortL("/dev/stdout", TFileAudioPort::OUTPUT);
    TFileAudioPort outputPortR("/dev/null", TFileAudioPort::OUTPUT);
    TAudioPortCollection outputPorts({&outputPortL, &outputPortR});

    TGlobal::SampleRate = 44100;
    TGlobal::NyquistFrequency = TGlobal::SampleRate / 2;
    TJackSynth synth(inputPorts, outputPorts);

    switch (testsignal) {
    case 1:
      testSignalSawSweep(synth);
      break;
    case 2:
      testSignalFilterSweep(synth);
      break;
    }
  } else {
    signal(SIGINT, sigterm);
    signal(SIGTERM, sigterm);
    runInJack();
  }
  return 0;
}
