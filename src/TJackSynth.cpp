#include "TJackSynth.h"

#include <iostream>

TChannel::TChannel()
    : ActiveProgram(), ZombiePrograms(), Active(true)
{
}

void TChannel::ChangeProgram(std::unique_ptr<TProgram>& p)
{
    if (ActiveProgram) {
        ZombiePrograms.push_front(std::move(ActiveProgram));
    }
    ActiveProgram = std::move(p);
}

void TChannel::NoteOn(TUnsigned7 note, TUnsigned7 velocity)
{
    if (ActiveProgram) {
        ActiveProgram->NoteOn(note, velocity);
    }
}
void TChannel::NoteOff(TUnsigned7 note, TUnsigned7 velocity)
{
    if (ActiveProgram) {
        ActiveProgram->NoteOff(note, velocity);
    }
    // Give all zombie programs a chance to release the note
    for (auto& p : ZombiePrograms) {
        p->NoteOff(note, velocity);
    }
}

/*
 * **********************************************************************
 */

TJackSynth::TJackSynth(TAudioPortCollection& inputPorts,
        TAudioPortCollection& outputPorts,
        TAudioPortCollection& intermediateOutPorts)
        : TimerProcess("Global process"), InputPorts(inputPorts), OutputPorts(outputPorts),
          IntOutPorts(intermediateOutPorts), ClockRecovery(), Channels(TGlobal::MidiChannels),
          SendMidi(0)
{
}

TJackSynth::~TJackSynth()
{
}

void TJackSynth::HandleMidi(std::vector<uint8_t> data, uint64_t timestamp)
{
    uint8_t channel = data[0] & 0x0f;
    TChannel& c = Channels[channel];

    if (data.size() == 0) {
        return;
    }
    else if (data[0] == MIDI_CLOCK_TICK) {
        ClockRecovery.Beat(timestamp);
    }
    else if (!c.Active) {
        return;
    }
    else if ((data[0] & 0xf0) == MIDI_PGMCHANGE) {
        printf("[C%d] Program change: %d\n", channel + 1, data[1]);
        std::unique_ptr<TProgram> newProgram(new TProgram(data[1]));
        newProgram->SetParameterChangedCallback([this](int unit, int param, int value)
                { ParameterChangedCallback(unit, param, value); });
        newProgram->DumpParameters();

        c.ChangeProgram(newProgram);
    }
    else if (!c.ActiveProgram) {
        return;
    }
    else if ((data[0] & 0xf0) == MIDI_NOTE_ON) {
        if (data[2] > 0) {
            c.NoteOn(data[1], data[2]);
        }
        else {
            c.NoteOff(data[1], data[2]);
        }
    }
    else if ((data[0] & 0xf0) == MIDI_NOTE_OFF) {
        c.NoteOff(data[1], data[2]);
    }
    else if ((data[0] & 0xf0) == MIDI_PITCHBEND) {
        // Pitch bend
        int bend = ((data[2] << 7) | (data[1])) - 0x2000;
        c.ActiveProgram->SetPitchBend((float) bend / 0x2000); // -1..1
    }
    else if ((data[0] & 0xf0) == MIDI_CC) {
        TUnsigned7 cc = data[1];
        TUnsigned7 value = data[2];
        if ((cc == MIDI_CC_SUSTAIN || cc == MIDI_CC_SOSTENUTO) && value == 0) {
            for (auto& p : c.ZombiePrograms) {
                p->SetController(cc, value);
            }
        }
        c.ActiveProgram->SetController(cc, value);
    }
    else if (data[0] == MIDI_SYSEX && data[1] == 0x7f) {
        // Set parameter sysex
        int param = data[2];
        int unit = data[3];
        int value = short((data[4] << 9) | (data[5] << 2)) >> 2;
        c.ActiveProgram->SetParameter(unit, static_cast<TParameter>(param), value);
    }
    else if (data[0] == MIDI_SYSEX && data[1] == 0x3e) {
        HandleWaldorfSysex(*c.ActiveProgram, data);
    }
    else if (data[0] == MIDI_SYSEX && data[1] == 0x7e) {
        c.ActiveProgram->DumpParameters();
    }
    else {
        //printf("[C%d] MIDI: %02x %02x %02x %02x\n", channel + 1, data[0], data[1], data[2], data[3]);
    }
}

static TOscType blofeldOscType(uint8_t v)
{
    switch (v) {
    case 0: return OSC_OFF;
    case 1: return OSC_SQUARE;
    case 2: return OSC_SAW;
    case 3: return OSC_OFF; // Triangle
    case 4: return OSC_SINE;
    default: return OSC_WT;
    }
}

void TJackSynth::HandleWaldorfSysex(TProgram& program, std::vector<uint8_t> data)
{
    if (data.size() < 10) {
        return;
    }

    uint8_t idm = data[4];
    if (idm == 0x20) {
        // Parameter change
        uint16_t n = data[6] << 7 | data[7];
        uint8_t v = data[8];

        printf("Waldorf sysex %d = %d\n", n, v);

        int signedvalue = v - 64;
        TFraction signedfractvalue = signedvalue / 64.0f;

        // Mapping from blofeld_sysex_v1_04.txt
        switch (n) {
        case   1: program.SetParameter(0, PARAM_OSC_OCTAVE, signedvalue, true); break;
        case   3: program.SetParameter(0, PARAM_OSC_DETUNE, 100*signedfractvalue, true); break;
        case   8: program.SetParameter(0, PARAM_OSC_TYPE, blofeldOscType(v), true); break;
        case   9: program.SetParameter(0, PARAM_PULSE_WIDTH, v, true); break;
        case  17: program.SetParameter(1, PARAM_OSC_OCTAVE, signedvalue, true); break;
        case  19: program.SetParameter(1, PARAM_OSC_DETUNE, 100*signedfractvalue, true); break;
        case  24: program.SetParameter(1, PARAM_OSC_TYPE, blofeldOscType(v), true); break;
        case  25: program.SetParameter(1, PARAM_PULSE_WIDTH, v, true); break;
        case  33: program.SetParameter(2, PARAM_OSC_OCTAVE, signedvalue, true); break;
        case  35: program.SetParameter(2, PARAM_OSC_DETUNE, 100*signedfractvalue, true); break;
        case  40: program.SetParameter(2, PARAM_OSC_TYPE, blofeldOscType(v), true); break;
        case  41: program.SetParameter(2, PARAM_PULSE_WIDTH, v, true); break;
        case  61: program.SetParameter(0, PARAM_OSC_LEVEL, v, true); break;
        case  63: program.SetParameter(1, PARAM_OSC_LEVEL, v, true); break;
        case  65: program.SetParameter(2, PARAM_OSC_LEVEL, v, true); break;

        case  78: program.SetParameter(0, PARAM_FILTER_CUTOFF_HZ, VAL2HZ_HI(v), true); break;
        case  80: program.SetParameter(0, PARAM_FILTER_RESONANCE, v, true); break;
        case  81: program.SetParameter(0, PARAM_DISTORTION, v, true); break;
        case  98: program.SetParameter(1, PARAM_FILTER_CUTOFF_HZ, VAL2HZ_HI(v), true); break;
        case 100: program.SetParameter(1, PARAM_FILTER_RESONANCE, v, true); break;
        case 101: program.SetParameter(1, PARAM_DISTORTION, v, true); break;
        case 129: program.SetParameter(0, PARAM_FX_MIX, v, true); break;
        case 133: program.SetParameter(0, PARAM_FX_FEEDBACK, v, true); break;
        case 134: program.SetParameter(0, PARAM_FX_DELAY, VAL2MS(v), true); break;
        case 145: program.SetParameter(1, PARAM_FX_MIX, v, true); break;
        case 149: program.SetParameter(1, PARAM_FX_FEEDBACK, v, true); break;
        case 150: program.SetParameter(1, PARAM_FX_DELAY, VAL2MS(v), true); break;
        case 161: program.SetParameter(0, PARAM_LFO_FREQUENCY_FRACHZ, VAL2HZ_LO(v)*128, true); break;
        case 173: program.SetParameter(1, PARAM_LFO_FREQUENCY_FRACHZ, VAL2HZ_LO(v)*128, true); break;

        case 199: program.SetParameter(0x10, PARAM_ENVELOPE, VAL2MS(v), true); break;
        case 201: program.SetParameter(0x11, PARAM_ENVELOPE, VAL2MS(v), true); break;
        case 202: program.SetParameter(0x12, PARAM_ENVELOPE, v, true); break;
        case 205: program.SetParameter(0x13, PARAM_ENVELOPE, VAL2MS(v), true); break;

        case 211: program.SetParameter(0x00, PARAM_ENVELOPE, VAL2MS(v), true); break;
        case 213: program.SetParameter(0x01, PARAM_ENVELOPE, VAL2MS(v), true); break;
        case 214: program.SetParameter(0x02, PARAM_ENVELOPE, v, true); break;
        case 217: program.SetParameter(0x03, PARAM_ENVELOPE, VAL2MS(v), true); break;
        }
    }
}

void TJackSynth::ParameterChangedCallback(int unit, int param, int value)
{
    if (!SendMidi) {
        return;
    }
    SendMidi({
        0xf0,
        0x7f,
        uint8_t(param),
        uint8_t(unit),
        uint8_t((value >> 7) & 0x7f),
        uint8_t(value & 0x7f), 0xf7 });
}

void TJackSynth::SetPitchBend(float bend)
{
    // For debugging only
    for (auto& c : Channels) {
        if (c.ActiveProgram) {
            c.ActiveProgram->SetPitchBend(bend);
        }
    }
}

void TJackSynth::SetMidiSendCallback(
        std::function<void(const std::vector<uint8_t>&)> cb)
{
    SendMidi = cb;
}

int TJackSynth::Process(jack_nframes_t nframes)
{
    TimerProcess.Start();

    TSampleBuffer in[2] = { InputPorts[0]->GetBuffer(nframes),
            InputPorts[1]->GetBuffer(nframes) };
    TSampleBuffer out[2] = { OutputPorts[0]->GetBuffer(nframes),
            OutputPorts[1]->GetBuffer(nframes) };
    TSampleBuffer into[4] = { IntOutPorts[0]->GetBuffer(nframes),
            IntOutPorts[1]->GetBuffer(nframes), IntOutPorts[2]->GetBuffer(nframes),
            IntOutPorts[3]->GetBuffer(nframes) };

    TSampleBufferCollection ins( { &in[0], &in[1] });
    TSampleBufferCollection outs( { &out[0], &out[1] });
    TSampleBufferCollection ints( { &into[0], &into[1], &into[2], &into[3] });

    out[0].Clear();
    out[1].Clear();
    into[0].Clear();
    into[1].Clear();
    into[2].Clear();
    into[3].Clear();

    for (auto& c : Channels) {
        if (c.ActiveProgram) {
            c.ActiveProgram->SetQuarterNoteTime(ClockRecovery.GetQuarterNoteTime());
            c.ActiveProgram->Process(ins, outs, ints);
        }
        auto p = c.ZombiePrograms.begin();
        while (p != c.ZombiePrograms.end()) {
            bool sounding = (*p)->Process(ins, outs, ints);
            auto oldp = p++;
            if (!sounding) {
                c.ZombiePrograms.erase(oldp);
            }
        }
    }

    OutputPorts[0]->Commit();
    OutputPorts[1]->Commit();
    IntOutPorts[0]->Commit();
    IntOutPorts[1]->Commit();
    IntOutPorts[2]->Commit();
    IntOutPorts[3]->Commit();

    TimerProcess.Stop();

    return 0;
}
