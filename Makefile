CCFLAGS=-g -Wall -O2 -std=c++0x `pkg-config --cflags jack sndfile` -Weffc++
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
SRCS += TSampleLoader.cpp
SRCS += TSampleOscillator.cpp
SRCS += TWavetableOscillator.cpp
SRCS += liir.cpp
SRCS += main.cpp
SRCS += util.cpp

OBJS = $(SRCS:%.cpp=build/%.o)

-include deps.mk

clean:
	rm -f $(EXE) $(OBJS) deps.mk
	rm -rf build

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
		OBJ=`echo $$s | sed s/\.cpp/\.o/`; \
		gcc -MT "$$OBJ" -MM $$s $(CCFLAGS) >> $@; \
	done



testspec: spectrogram.m $(EXE)
	./$(EXE) --testsignal > testdata.bin && octave --persist spectrogram.m

testspeed: $(EXE)
	/usr/bin/time ./$(EXE) --testsignal > /dev/null

minBLEP/minblep: minBLEP/minblep.o minBLEP/main.o
	$(CXX) -o $@ $^

showbleps: minBLEP/minblep
	minBLEP/minblep --zerocrossings 48 --oversampling 32 > minBLEP/data.txt
	octave --persist minBLEP/illustration.m

minblep.h: minBLEP/minblep #Makefile
	minBLEP/minblep --zerocrossings 16 --oversampling 64 --header > $@

.PHONY: testspec showbleps
