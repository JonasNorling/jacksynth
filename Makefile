HAVE_LIBGIG := $(shell pkg-config --exists gig && echo y)
PKGS := jack sndfile jsoncpp $(if $(HAVE_LIBGIG),gig)

CCFLAGS := -g -Wall -O3 -std=c++11 $(shell pkg-config --cflags $(PKGS))
CCFLAGS += $(if $(HAVE_LIBGIG),-DHAVE_LIBGIG=1)
CCFLAGS += -Wno-deprecated-register
#CCFLAGS += -msse2 -fopt-info-vec-optimized
#CCFLAGS += -Weffc++
#CCFLAGS += -fno-exceptions -fno-rtti
#CCFLAGS += -Winline
LDFLAGS := -lm $(shell pkg-config --libs $(PKGS))

EXE = jacksynth
all: $(EXE)

SRCS += src/TAudioFileWriter.cpp
SRCS += src/TButterworthLpFilter.cpp
SRCS += src/TClockRecovery.cpp
SRCS += src/TDelayFx.cpp
SRCS += src/TEnvelope.cpp
SRCS += src/TFileAudioPort.cpp
ifeq ($(HAVE_LIBGIG),y)
SRCS += src/TGigInstrument.cpp
SRCS += src/TGigOscillator.cpp
endif
SRCS += src/TJackAudioPort.cpp
SRCS += src/TJackSynth.cpp
SRCS += src/TMinBlepPulseOscillator.cpp
SRCS += src/TMinBlepSawOscillator.cpp
SRCS += src/TProgram.cpp
SRCS += src/TResonantLpFilter.cpp
SRCS += src/TReverbFx.cpp
SRCS += src/TSampleLoader.cpp
SRCS += src/TSampleOscillator.cpp
SRCS += src/TSpeedTest.cpp
SRCS += src/TSvfFilter.cpp
SRCS += src/TWavetableOscillator.cpp
SRCS += src/liir.cpp
SRCS += src/main.cpp
SRCS += src/util.cpp

OBJS := $(patsubst src/%.cpp,build/%.o,$(SRCS))

-include $(patsubst %.o,%.d,$(OBJS))

clean:
	rm -f $(EXE) $(OBJS)
	rm -rf build
	rm -f src/minblep.h

$(EXE): $(OBJS)
	@echo --\> $@
	@$(CXX) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.cpp src/minblep.h
	@echo $< --\> $@
	@mkdir -p build
	@$(CXX) -MMD -c -o $@ $< $(CCFLAGS)

testsignal1.wav: $(EXE)
	./$(EXE) --testsignal 1
	mv testsignal.wav $@

testsignal2.wav: $(EXE)
	./$(EXE) --testsignal 2
	mv testsignal.wav $@

testsignal3.wav: $(EXE)
	./$(EXE) --testsignal 3
	mv testsignal.wav $@

testsignal4.wav: $(EXE)
	./$(EXE) --testsignal 4
	mv testsignal.wav $@

%.wav: %.bin
	sox -c 1 -r 44100 -t f32 $< $@

%.bin: %.wav
	sox $< -t f32 -c 1 $@

testspec1: spectrogram.m $(EXE) testsignal1.bin testsignal1.wav
	octave --persist --eval "filename='testsignal1.bin'" spectrogram.m

testspec2: spectrogram.m $(EXE) testsignal2.bin testsignal2.wav
	octave --persist --eval "filename='testsignal2.bin'" spectrogram.m

testspeed: $(EXE)
	/usr/bin/time ./$(EXE) --testsignal 2 > /dev/null

minBLEP/minblep: minBLEP/minblep.o minBLEP/main.o
	$(CXX) -o $@ $^

showbleps: minBLEP/minblep
	minBLEP/minblep --zerocrossings 32 --oversampling 32 --type minblep > minBLEP/data.txt
	octave --persist minBLEP/illustration.m

src/minblep.h: minBLEP/minblep
	minBLEP/minblep --zerocrossings 32 --oversampling 64 --header --type minblep > $@

.PHONY: testspec showbleps
