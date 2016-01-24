# joy2midi

This is a simple utility for Linux to map joystick events to MIDI events
in [JACK](http://www.jackaudio.org/). Install it like this on Ubuntu:

```
$ sudo apt-get install build-essential libjack-jackd2-dev
$ git clone https://github.com/jessecrossen/hautmidi.git
$ cd ./hautmidi/joy2midi/
$ make && sudo make install
```

Since there's no standardized mapping from joysticks to MIDI devices, 
you'll need to make one up. This program supports a fairly flexible
format for map files. Joysticks have buttons (which are on or off) 
and axes (which go more continuously from -32767 to 32767). MIDI has 
numbered notes, pitch bends, and control changes. Each line in the map file will 
map one of the things in the first list to one of the things in the 
second list, like this:

```
# (this is a comment and does nothing)
# map button 1 to middle C
button 1 => note 60
# map axis 2 to modulation
axis 2 => control 1
# map axis 0 to pitch bend
axis 0 => bend
# ignore input from axis 1
axis 1 => ignore
```

There's more detail about map files below, but once you've made one, you
need to find which device represents your joystick (it should be something
like `/dev/input/js<N>`). You'll also need to have JACK started. Now you
should be able to do this:

```
$ joy2midi my.map /dev/input/js0
```

You'll see joy2midi as an input device in JACK, which you can then connect
to your DAW, plugin, or soft-synth of choice. Enjoy!

# Map File Syntax

You can map specific input ranges to specific output ranges.
For example, suppose you want to map one half of an axis to one 
controller and the other half to the other:

```
axis 1 [0..32767] => control 1 [0..127]
axis 1 [-32767..0] => control 2 [127..0]
```

Or reverse which direction is sharp and flat for a pitch bend:

```
axis 0 [0..32767] => bend [8192..0]
axis 0 [-32767..0] => bend [16383..8192]
```

There are also some settings you can specify:

```
# no output unless there's an error
verbosity = 0
# show unmapped joystick events when running
verbosity = 2
# show MIDI output when running
verbosity = 3
# send output on MIDI channel 2
#  (channel numbers go from 1 to 16)
channel = 2
# send consecutive button clicks only if they're at least 0.25 seconds apart
#  (debounce time is specified in milliseconds)
debounce = 250
```

You should be able to hack around to discover what's possible, or examine 
the [bison](https://en.wikipedia.org/wiki/GNU_bison) generated [parser 
here](https://github.com/jessecrossen/hautmidi/blob/master/joy2midi/parser.c).
