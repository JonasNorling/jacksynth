CCFLAGS=-g -Wall -O2 -std=c++11 `pkg-config --cflags jack sndfile jsoncpp` -Weffc++
#CCFLAGS+=-fno-exceptions -fno-rtti
#CCFLAGS+=-Winline
LDFLAGS=-lm `pkg-config --libs jack sndfile gig jsoncpp` -lrt

EXE = jacksynth
all: $(EXE)

SRCS += src/TButterworthLpFilter.cpp
SRCS += src/TClockRecovery.cpp
SRCS += src/TDelayFx.cpp
SRCS += src/TEnvelope.cpp
SRCS += src/TFileAudioPort.cpp
SRCS += src/TGigInstrument.cpp
SRCS += src/TGigOscillator.cpp
SRCS += src/TJackAudioPort.cpp
SRCS += src/TJackSynth.cpp
SRCS += src/TMinBlepPulseOscillator.cpp
SRCS += src/TMinBlepSawOscillator.cpp
SRCS += src/TProgram.cpp
SRCS += src/TResonantLpFilter.cpp
SRCS += src/TReverbFx.cpp
SRCS += src/TSampleLoader.cpp
SRCS += src/TSampleOscillator.cpp
SRCS += src/TSvfFilter.cpp
SRCS += src/TWavetableOscillator.cpp
SRCS += src/liir.cpp
SRCS += src/main.cpp
SRCS += src/util.cpp

OBJS = $(SRCS:src/%.cpp=build/%.o)

-include deps.mk

clean:
	rm -f $(EXE) $(OBJS) deps.mk
	rm -rf build
	rm -f src/minblep.h

$(EXE): $(OBJS)
	@echo --\> $@
	@$(CXX) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.cpp
	@echo $< --\> $@
	@mkdir -p build
	@$(CXX) -c -o $@ $< $(CCFLAGS)

deps.mk: $(SRCS) $(TEST_SRCS) src/minblep.h
	@echo --\> $@
	@rm -f $@
	@touch deps.mk

	@for s in $(SRCS) $(TEST_SRCS); do \
		OBJ=`echo $$s | sed 's/src\/\(.*\).cpp/build\/\1.o/'`; \
		gcc -MT "$$OBJ" -MM $$s $(CCFLAGS) >> $@; \
	done


spectrogramdata1.bin: $(EXE)
	./$(EXE) --testsignal 1 > $@

spectrogramdata2.bin: $(EXE)
	./$(EXE) --testsignal 2 > $@

%.wav: %.bin
	sox -c 1 -r 44100 -t f32 $< $@

testspec1: spectrogram.m $(EXE) spectrogramdata1.bin spectrogramdata1.wav
	octave --persist --eval "filename='spectrogramdata1.bin'" spectrogram.m

testspec2: spectrogram.m $(EXE) spectrogramdata2.bin spectrogramdata2.wav
	octave --persist --eval "filename='spectrogramdata2.bin'" spectrogram.m

testspeed: $(EXE)
	/usr/bin/time ./$(EXE) --testsignal > /dev/null

minBLEP/minblep: minBLEP/minblep.o minBLEP/main.o
	$(CXX) -o $@ $^

showbleps: minBLEP/minblep
	minBLEP/minblep --zerocrossings 32 --oversampling 32 --type minblep > minBLEP/data.txt
	octave --persist minBLEP/illustration.m

src/minblep.h: minBLEP/minblep Makefile
	minBLEP/minblep --zerocrossings 32 --oversampling 64 --header --type minblep > $@

.PHONY: testspec showbleps
