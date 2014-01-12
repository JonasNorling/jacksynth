CCFLAGS=-g -Wall -O2 -std=c++11 `pkg-config --cflags jack sndfile` -Weffc++
CCFLAGS+=-fno-exceptions -fno-rtti
#CCFLAGS+=-Winline
LDFLAGS=-lm `pkg-config --libs jack sndfile` -lrt

EXE = jacksynth
all: $(EXE)

SRCS += TButterworthLpFilter.cpp
SRCS += TDelayFx.cpp
SRCS += TEnvelope.cpp
SRCS += TFileAudioPort.cpp
SRCS += TJackAudioPort.cpp
SRCS += TJackSynth.cpp
SRCS += TMinBlepPulseOscillator.cpp
SRCS += TMinBlepSawOscillator.cpp
SRCS += TProgram.cpp
SRCS += TResonantLpFilter.cpp
SRCS += TReverbFx.cpp
SRCS += TSampleLoader.cpp
SRCS += TSampleOscillator.cpp
SRCS += TSvfFilter.cpp
SRCS += TWavetableOscillator.cpp
SRCS += liir.cpp
SRCS += main.cpp
SRCS += util.cpp

OBJS = $(SRCS:%.cpp=build/%.o)

-include deps.mk

clean:
	rm -f $(EXE) $(OBJS) deps.mk
	rm -rf build
	rm minblep.h

$(EXE): $(OBJS)
	@echo --\> $@
	@$(CXX) -o $@ $^ $(LDFLAGS)

build/%.o: %.cpp
	@echo $< --\> $@
	@mkdir -p build
	@$(CXX) -c -o $@ $< $(CCFLAGS)

deps.mk: $(SRCS) $(TEST_SRCS)
	@echo --\> $@
	@rm -f $@
	@touch deps.mk

	@for s in $^; do \
		OBJ=build/`echo $$s | sed s/\.cpp/\.o/`; \
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

minblep.h: minBLEP/minblep Makefile
	minBLEP/minblep --zerocrossings 32 --oversampling 64 --header --type minblep > $@

.PHONY: testspec showbleps
