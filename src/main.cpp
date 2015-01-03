#include <unistd.h>
#include <getopt.h>
#include <iostream>
#include <signal.h>
#include <queue>

#include "TAudioFileWriter.h"
#include "TJackSynth.h"
#include "TFileAudioPort.h"
#include "TGlobal.h"
#include "TSpeedTest.h"

unsigned TGlobal::SampleRate;
unsigned TGlobal::NyquistFrequency;

jack_client_t* Client;
jack_port_t* MidiIn;
jack_port_t* MidiOut;
std::queue<std::vector<uint8_t> > MidiOutQueue;
TTimer TimerProcess("Jack CB");

volatile bool die = false;
void sigterm(int)
{
    die = true;
}

int process(jack_nframes_t nframes, void* arg)
{
    TimerProcess.Start();

    static uint64_t frametime = 0;
    TJackSynth* synth = static_cast<TJackSynth*>(arg);

    /* Process MIDI input */
    void* inbuf = jack_port_get_buffer(MidiIn, nframes);
    jack_nframes_t event_count = jack_midi_get_event_count(inbuf);
    for (jack_nframes_t i = 0; i < event_count; i++) {
        jack_midi_event_t event;
        jack_midi_event_get(&event, inbuf, i);
        std::vector<uint8_t> data(event.buffer, event.buffer + event.size);
        synth->HandleMidi(data, frametime + event.time);
    }

    /* Send MIDI */
    void* outbuf = jack_port_get_buffer(MidiOut, nframes);
    jack_midi_clear_buffer(outbuf);
    while (!MidiOutQueue.empty()) {
        const std::vector<uint8_t>& data = MidiOutQueue.front();
        int ret = jack_midi_event_write(outbuf, 0, data.data(), data.size());
        MidiOutQueue.pop();
        if (ret != 0) {
            fprintf(stderr, "MIDI send error\n");
        }
    }

    synth->Process(nframes);

    frametime += nframes;
    TimerProcess.Stop();

    return 0;
}

void send_midi(const std::vector<uint8_t>& data)
{
    MidiOutQueue.push(data);
}

void testSignalSawSweep(TJackSynth& synth)
{
    // This code depends on the default patch being a fairly
    // clean saw wave.

    const unsigned chunk = 16;

    // Program change
    synth.HandleMidi({0xc0, 0x00});

    TTimer timer("Testsignal");
    timer.Start();

    synth.HandleMidi({0x90, 0, 0x40}); // note 0: 8.18 Hz
    for (int i = 0; i < 44100 * 0.5; i += chunk) synth.Process(chunk);
    synth.HandleMidi({0x80, 0, 0x40}); // release note

    // Sweep
    synth.HandleMidi({0x90, 33, 0x40}); // note 33: A 55Hz
    for (int i = 0; i < 44100 * 0.2; i += chunk) synth.Process(chunk);
    for (float octave = 0; octave < 6; octave += 0.0005) {
        synth.SetPitchBend(octave * 6);
        synth.Process(chunk);
    }
    for (int i = 0; i < 44100 * 0.2; i += chunk) synth.Process(chunk);
    synth.HandleMidi({0x80, 33, 0x40}); // release note
    for (int i = 0; i < 44100 * 0.2; i += chunk) synth.Process(chunk);

    timer.Stop();
}

void testSignalFilterSweep(TJackSynth& synth)
{
    const float speed = 1.001f;
    const unsigned chunk = 32;

    // Program change
    synth.HandleMidi({0xc0, 0x00});

    TTimer timer("Testsignal");
    timer.Start();

    int value = 8000;
    synth.HandleMidi({0xf0, 0x7f, PARAM_DISTORTION, 1, hi7(value), lo7(value), 0xf7});
    value = 8000;
    synth.HandleMidi({0xf0, 0x7f, PARAM_FILTER_CUTOFF_HZ, 0, hi7(value), lo7(value), 0xf7});
    synth.HandleMidi({0xf0, 0x7f, PARAM_FILTER_CUTOFF_HZ, 1, hi7(value), lo7(value), 0xf7});
    value = 0;
    synth.HandleMidi({0xf0, 0x7f, PARAM_FILTER_RESONANCE, 0, hi7(value), lo7(value), 0xf7});
    synth.HandleMidi({0xf0, 0x7f, PARAM_FILTER_RESONANCE, 1, hi7(value), lo7(value), 0xf7});

    synth.HandleMidi({0x90, TGlobal::MidiNoteA4, 0x70});
    synth.HandleMidi({0x90, TGlobal::MidiNoteA4 - 3*12, 0x70});

    for (int i = 0; i < 44100 * 0.5; i += chunk) synth.Process(chunk);

    /* Low Q */
    value = 0;
    synth.HandleMidi({0xf0, 0x7f, PARAM_FILTER_RESONANCE, 0, hi7(value), lo7(value), 0xf7});
    synth.HandleMidi({0xf0, 0x7f, PARAM_FILTER_RESONANCE, 1, hi7(value), lo7(value), 0xf7});

    for (float hz = 10; hz < 8000; hz *= speed) {
        synth.HandleMidi({0xf0, 0x7f, PARAM_FILTER_CUTOFF_HZ, 0, hi7(hz), lo7(hz), 0xf7});
        synth.HandleMidi({0xf0, 0x7f, PARAM_FILTER_CUTOFF_HZ, 1, hi7(hz), lo7(hz), 0xf7});
        synth.Process(chunk);
    }

    for (int i = 0; i < 44100 * 0.5; i += chunk) synth.Process(chunk);

    /* Medium Q */
    value = 12;
    synth.HandleMidi({0xf0, 0x7f, PARAM_FILTER_RESONANCE, 0, hi7(value), lo7(value), 0xf7});
    synth.HandleMidi({0xf0, 0x7f, PARAM_FILTER_RESONANCE, 1, hi7(value), lo7(value), 0xf7});

    for (float hz = 10; hz < 8000; hz *= speed) {
        synth.HandleMidi({0xf0, 0x7f, PARAM_FILTER_CUTOFF_HZ, 0, hi7(hz), lo7(hz), 0xf7});
        synth.HandleMidi({0xf0, 0x7f, PARAM_FILTER_CUTOFF_HZ, 1, hi7(hz), lo7(hz), 0xf7});
        synth.Process(chunk);
    }

    for (int i = 0; i < 44100 * 0.5; i += chunk) synth.Process(chunk);

    /* High Q */
    value = 127;
    synth.HandleMidi({0xf0, 0x7f, PARAM_FILTER_RESONANCE, 0, hi7(value), lo7(value), 0xf7});
    synth.HandleMidi({0xf0, 0x7f, PARAM_FILTER_RESONANCE, 1, hi7(value), lo7(value), 0xf7});

    for (float hz = 10; hz < 8000; hz *= speed) {
        synth.HandleMidi({0xf0, 0x7f, PARAM_FILTER_CUTOFF_HZ, 0, hi7(hz), lo7(hz), 0xf7});
        synth.HandleMidi({0xf0, 0x7f, PARAM_FILTER_CUTOFF_HZ, 1, hi7(hz), lo7(hz), 0xf7});
        synth.Process(chunk);
    }

    for (int i = 0; i < 44100 * 0.5; i += chunk) synth.Process(chunk);

    timer.Stop();
}

void testSignalDelay(TJackSynth& synth)
{
    const unsigned chunk = 16;

    // Program change
    synth.HandleMidi({0xc0, 0x01});

    // Open up the filter
    unsigned value = 8000;
    synth.HandleMidi({0xf0, 0x7f, PARAM_FILTER_CUTOFF_HZ, 0, hi7(value), lo7(value), 0xf7});
    synth.HandleMidi({0xf0, 0x7f, PARAM_FILTER_CUTOFF_HZ, 1, hi7(value), lo7(value), 0xf7});

    // Set delay parameters
    value = 500;
    synth.HandleMidi({0xf0, 0x7f, PARAM_FX_DELAY, 0, hi7(value), lo7(value), 0xf7});
    value = 100;
    synth.HandleMidi({0xf0, 0x7f, PARAM_FX_FEEDBACK, 0, hi7(value), lo7(value), 0xf7});
    value = 64;
    synth.HandleMidi({0xf0, 0x7f, PARAM_FX_MIX, 0, hi7(value), lo7(value), 0xf7});

    // Set release time
    value = 10;
    synth.HandleMidi({0xf0, 0x7f, PARAM_ENVELOPE, 0x03, hi7(value), lo7(value), 0xf7});
    synth.HandleMidi({0xf0, 0x7f, PARAM_ENVELOPE, 0x13, hi7(value), lo7(value), 0xf7});

    TTimer timer("Testsignal");
    timer.Start();

    synth.HandleMidi({0x90, TGlobal::MidiNoteA4, 0x40});
    synth.HandleMidi({0x90, TGlobal::MidiNoteA4 + 6, 0x40});
    for (int i = 0; i < 44100 * 0.05; i += chunk) synth.Process(chunk);
    synth.HandleMidi({0x80, TGlobal::MidiNoteA4, 0x40});
    synth.HandleMidi({0x80, TGlobal::MidiNoteA4 + 6, 0x40});

    for (int i = 0; i < 44100 * 1; i += chunk) synth.Process(chunk);

    // Set delay in ms
    value = 312;
    synth.HandleMidi({0xf0, 0x7f, PARAM_FX_DELAY, 0, hi7(value), lo7(value), 0xf7});

    for (int i = 0; i < 44100 * 2; i += chunk) synth.Process(chunk);

    // Set delay in ms
    value = 206;
    synth.HandleMidi({0xf0, 0x7f, PARAM_FX_DELAY, 0, hi7(value), lo7(value), 0xf7});

    for (int i = 0; i < 44100 * 2; i += chunk) synth.Process(chunk);

    timer.Stop();
}

void testSignalReverb(TJackSynth& synth)
{
    const unsigned chunk = 16;

    // Program change
    synth.HandleMidi({0xc0, 0x00});

    unsigned value = 1; // Reverb
    synth.HandleMidi({0xf0, 0x7f, PARAM_FX_TYPE, 0, hi7(value), lo7(value), 0xf7});
    TTimer timer("Testsignal");
    timer.Start();

    auto signal = [&synth]() {
        synth.HandleMidi({0x90, 120, 0x70});
        for (float octave = 0; octave < 6; octave += 0.03) {
            synth.SetPitchBend(octave * -6);
            synth.Process(chunk);
        }
        synth.HandleMidi({0x80, 120, 0x40}); // release note
        for (int i = 0; i < 44100 * 1.0; i += chunk) synth.Process(chunk);
    };

    value = 0;
    synth.HandleMidi({0xf0, 0x7f, PARAM_FX_MIX, 0, hi7(value), lo7(value), 0xf7});
    signal();

    value = 32;
    synth.HandleMidi({0xf0, 0x7f, PARAM_FX_MIX, 0, hi7(value), lo7(value), 0xf7});
    signal();

    value = 64;
    synth.HandleMidi({0xf0, 0x7f, PARAM_FX_MIX, 0, hi7(value), lo7(value), 0xf7});
    signal();

    value = 127;
    synth.HandleMidi({0xf0, 0x7f, PARAM_FX_MIX, 0, hi7(value), lo7(value), 0xf7});
    signal();

    for (int i = 0; i < 44100 * 1; i += chunk) synth.Process(chunk);

    timer.Stop();
}

static void speedTest()
{
    TGlobal::SampleRate = 48000;
    TGlobal::NyquistFrequency = TGlobal::SampleRate / 2;
    TSpeedTest speedTest;
    speedTest.Run();
}

void runInJack(bool connectAudio,
		std::vector<std::string> midiConnections,
		std::map<unsigned, unsigned> patchFromCmdline)
{
    Client = jack_client_open("jacksynth", JackNullOption, 0);
    if (!Client) {
        std::cerr << "Cannot connect to JACK" << std::endl;
        abort();
    }

    std::cout << "Connected to JACK:" << std::endl << "  Sample rate "
            << jack_get_sample_rate(Client) << " Hz" << std::endl
            << "  Buffer size " << jack_get_buffer_size(Client) << " frames"
            << std::endl << "  Sample size " << sizeof(TSample) << " bytes"
            << std::endl;

    TJackAudioPort inputPortL("leftin", Client, TJackAudioPort::INPUT);
    TJackAudioPort inputPortR("rightin", Client, TJackAudioPort::INPUT);
    TJackAudioPort outputPortL("left", Client, TJackAudioPort::OUTPUT);
    TJackAudioPort outputPortR("right", Client, TJackAudioPort::OUTPUT);
    TJackAudioPort intOutPort1("int1", Client, TJackAudioPort::OUTPUT);
    TJackAudioPort intOutPort2("int2", Client, TJackAudioPort::OUTPUT);
    TJackAudioPort intOutPort3("int3", Client, TJackAudioPort::OUTPUT);
    TJackAudioPort intOutPort4("int4", Client, TJackAudioPort::OUTPUT);

    MidiIn = jack_port_register(Client, "MIDI-IN",
            JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);

    MidiOut = jack_port_register(Client, "MIDI-OUT",
            JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);

    TGlobal::SampleRate = jack_get_sample_rate(Client);
    TGlobal::NyquistFrequency = TGlobal::SampleRate / 2;

    TAudioPortCollection inputPorts( { &inputPortL, &inputPortR });
    TAudioPortCollection outputPorts( { &outputPortL, &outputPortR });
    TAudioPortCollection intermediateOutPorts( { &intOutPort1, &intOutPort2,
            &intOutPort3, &intOutPort4 });
    TJackSynth synth(inputPorts, outputPorts, intermediateOutPorts);
    synth.SetMidiSendCallback(send_midi);

    jack_set_process_callback(Client, process, &synth);
    if (jack_activate(Client)) {
        std::cerr << "Cannot start jackiness" << std::endl;
        abort();
    }

    if (connectAudio) {
        inputPortL.Connect("system:capture_1");
        inputPortR.Connect("system:capture_2");
        outputPortL.Connect("system:playback_1");
        outputPortR.Connect("system:playback_2");
    }

    for (auto con : midiConnections) {
        const char* me = jack_port_name(MidiIn);
        jack_connect(Client, con.c_str(), me);
    }

    for (auto patch : patchFromCmdline) {
        uint8_t channel = 0xc0 | (patch.first - 1);
        uint8_t pgm = patch.second - 1;
        synth.HandleMidi({ channel, pgm });
    }

    while (!die)
        sleep(1);
    jack_client_close(Client);
}

static void printHelp()
{
    std::cout << "Usage: jacksynth [<args...>]" << std::endl
            << " Where allowed arguments are:" << std::endl
            << "   -t, --testsignal <N>   Generate testsignal number N, to file testsignal" << std::endl
            << "   -s, --speedtest        Run the performance test benchmark" << std::endl
            << "   -C, --connect-audio    Auto-connect audio in and out ports" << std::endl
            << "   -M, --connect-midi <P> Connect MIDI input to JACK port P" << std::endl
            << "   -p, --patch <ch>:<pgm> Set program on MIDI channel (1-indexed)" << std::endl
            << "   -h, --help             Print this help text"
            << std::endl << std::endl;
}

int main(int argc, char* argv[])
{
    int testsignal = 0;
    bool runSpeedTest = false;
    bool connectAudio = false;
    std::vector<std::string> midiConnections;
    std::map<unsigned, unsigned> patchFromCmdline;

    struct option longopts[] = {
            { "testsignal", 1, 0, 't' },
            { "speedtest", 0, 0, 's' },
            { "connect-audio", 0, 0, 'C' },
            { "connect-midi", 1, 0, 'M' },
            { "patch", 1, 0, 'p' },
            { "help", no_argument, 0, 'h' },
            { 0, 0, 0, 0 } };
    int opt;
    while ((opt = getopt_long(argc, argv, "st:CM:p:h", longopts, 0)) != -1) {
        switch (opt) {
        case 't':
            testsignal = atoi(optarg);
            break;
        case 's':
            runSpeedTest = true;
            break;
        case 'C':
            connectAudio = true;
            break;
        case 'M':
            midiConnections.push_back(optarg);
            break;
        case 'p':
            unsigned channel, program;
            sscanf(optarg, "%u:%u", &channel, &program);
            patchFromCmdline[channel] = program;
            break;
        default:
        case 'h':
            printHelp();
            return 0;
        }
    }

    if (testsignal) {
        TGlobal::SampleRate = 44100;
        TGlobal::NyquistFrequency = TGlobal::SampleRate / 2;

        TAudioFileWriter fileWriter("testsignal.wav", 2, TGlobal::SampleRate);

        TFileAudioPort inputPortL("", TFileAudioPort::INPUT);
        TFileAudioPort inputPortR("", TFileAudioPort::INPUT);
        TAudioPortCollection inputPorts( { &inputPortL, &inputPortR });
        TFileAudioPort intOutPort1("/dev/null", TFileAudioPort::OUTPUT);
        TFileAudioPort intOutPort2("/dev/null", TFileAudioPort::OUTPUT);
        TFileAudioPort intOutPort3("/dev/null", TFileAudioPort::OUTPUT);
        TFileAudioPort intOutPort4("/dev/null", TFileAudioPort::OUTPUT);
        TAudioPortCollection intOutPorts( { &intOutPort1, &intOutPort2,
                &intOutPort3, &intOutPort4 });
        TAudioPortCollection outputPorts = fileWriter.GetPorts();

        TJackSynth synth(inputPorts, outputPorts, intOutPorts);

        switch (testsignal) {
        case 1:
            testSignalSawSweep(synth);
            break;
        case 2:
            testSignalFilterSweep(synth);
            break;
        case 3:
            testSignalDelay(synth);
            break;
        case 4:
            testSignalReverb(synth);
            break;
        }
    }
    else if (runSpeedTest) {
        speedTest();
    }
    else {
        signal(SIGINT, sigterm);
        signal(SIGTERM, sigterm);
        runInJack(connectAudio, midiConnections, patchFromCmdline);
    }
    return 0;
}
