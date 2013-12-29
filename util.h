/* -*- mode: c++ -*- */
#pragma once

#include <cstdint>
#include <string>
#include <time.h>
#include "TGlobal.h"

#define UNCOPYABLE(T) \
  private: \
  T(const T&) = delete; \
  T& operator=(const T&) = delete;

typedef uint8_t TUnsigned7; // Value 0..127
typedef uint8_t TSigned7;   // Value -64..63
typedef float TFraction;    // Value 0..1
typedef float TFrequency;   // in Hz
typedef float TTime;        // in ms

static inline float octaves(float x) { return x; }
static inline float semitones(float x) { return x/12; }
static inline float cents(float x) { return x/1200; }
static inline int ms_to_samples(int x) { return (TGlobal::SampleRate * x) / 1000; }

class TTimer
{
  UNCOPYABLE(TTimer);
public:
  TTimer(const std::string& name);
  ~TTimer();

  void Start()
  {
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &StartTime);
  }
  void Stop()
  {
    struct timespec stopTime;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stopTime);
    Nanoseconds += (stopTime.tv_sec - StartTime.tv_sec) * 1000000000 +
      (stopTime.tv_nsec - StartTime.tv_nsec);
  }
  long long unsigned GetMs()
  {
    return Nanoseconds/1000000;
  }

private:
  std::string Name;
  struct timespec StartTime;
  long long unsigned Nanoseconds;
};

template<typename T>
T clamp(T v, T min, T max) { return std::max(std::min(v, max), min); }
