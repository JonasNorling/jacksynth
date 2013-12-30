JACK synth experiment
=====================

This is an experiment with building a sound synthesizer on JACK. Several oscillators, modulations and filters are implemented, with different degrees of maturity.


Building
========

make


Playing
=======

Hook up a MIDI keyboard, use JACK to route MIDI and audio. Program change messages can be used to trigger presets, MIDI sysex for parameter settings (see the files in patchpirate/ for a summary).


Evaluating performance
======================

Baudline is quite useful to draw realtime spectrograms, for rooting out aliasing and comparing to other synths.


References
==========

Eli Brandt: Hard Sync Without Aliasing

http://www.experimentalscene.com/articles/minbleps.php (MinBLEP Generation Code, by Daniel Werner)


Interresting reading
====================

Jussi Pekonen: Computationally Efficient Music Synthesis – Methods and Sound Design (2007)
