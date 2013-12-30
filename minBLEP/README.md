minBLEPs for audio synthesis
============================

minBLEPs are minimum-phase band-limited steps. minBLEPs are constructed from a windowed sinc (BLEP) that has undergone a conversion to minimum-phase, thus placing its energy hump at the beginning of the window instead of the middle of the window.

We implement pulse (generalized square wave) and saw oscillators using minBLEPs. The general idea is to avoid the aliasing that a naive implementation would suffer from by composing the waveforms from a band limited building block. This is where the band limited step (BLEP) comes in.

The oscillators work by adding minBLEPs onto a buffer at the appropriate times. Sub-sample accuracy when placing the BLEP is achieved by picking samples from an over-sampled pre-calculated BLEP. Too course-grained sub-sample accuracy would introduce phase noise in the oscillator, and it would sound like shit.


Building
========

cd ../
make minBLEP/minblep
make minblep.h
make showbleps


References
==========

Eli Brandt: Hard Sync Without Aliasing

http://www.experimentalscene.com/articles/minbleps.php (MinBLEP Generation Code, by Daniel Werner)
