/* -*- mode: c++ -*- */
#pragma once

#include <cstdint>

namespace TGlobal
{
extern int SampleRate;
extern int NyquistFrequency;

const float OscAmplitude = 1.0 / 8;

const int MidiNoteA4 = 69;
const int MidiNoteE3 = 64; ///< Centre note
const int MidiNoteC0 = 12;
const int HzA4 = 440;
const int HzE3 = 330;
const int MidiChannels = 16;

const unsigned SoftVoiceLimit = 16;

const int Oscillators = 3;
const int Filters = 2;
const int Lfos = 2;
const int Effects = 1;
}

static const uint8_t MIDI_NOTE_OFF = 0x80;
static const uint8_t MIDI_NOTE_ON = 0x90;
static const uint8_t MIDI_CC = 0xb0;
static const uint8_t MIDI_PGMCHANGE = 0xc0;
static const uint8_t MIDI_PITCHBEND = 0xe0;

static const uint8_t MIDI_SYSEX = 0xf0;
static const uint8_t MIDI_END_SYSEX = 0xf7;

static const uint8_t MIDI_CLOCK_TICK = 0xf8;
static const uint8_t MIDI_SEQ_START = 0xfa;
static const uint8_t MIDI_SEQ_CONTINUE = 0xfb;
static const uint8_t MIDI_SEQ_STOP = 0xfc;
static const uint8_t MIDI_ACTIVE_SENSE = 0xfe;
static const uint8_t MIDI_RESET = 0xff;

static const uint8_t MIDI_REALTIME_MASK = 0xf8;

static const uint8_t MIDI_CC_SUSTAIN = 64;
static const uint8_t MIDI_CC_SOSTENUTO = 66;
