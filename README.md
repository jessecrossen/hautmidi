# HAUT MIDI
This is a collection of experimental MIDI input devices I'm working on. The Wild West of the MIDI protocol 
still hasn't been fully explored, and scores are settled with [cables](https://en.wikipedia.org/wiki/MIDI) 
and [keytars](https://en.wikipedia.org/wiki/Keytar)...

![HAUT MIDI](https://raw.githubusercontent.com/jessecrossen/hautmidi/master/poster/poster.png)

# Joysticks!

A quick and easy way to start making interesting MIDI signals is to use a joystick or joystick-compatible 
device mapped to MIDI with my [joy2midi](joy2midi/README.md) utility. I've tried it with a [gaming joystick](http://www.amazon.com/gp/product/B00009OY9U), but there are more exotic options like 
[3D mice](http://www.3dconnexion.com/products/spacemouse.html), which act like 6-axis joysticks.
All the controllers for the [RockBand](https://en.wikipedia.org/wiki/Rock_Band_%28video_game%29)
game are actually just joysticks with interesting shapes. There are probably many more out there
if you look.

# Custom Electronics!

The [tonestrip](tonestrip/README.md) is the first piece of electronic hardware I've built in a long time.
Man, things have come a long way since I last burned my fingers on a soldering iron! For one thing I finally 
discovered the joys of [wire wrap](https://en.wikipedia.org/wiki/Wire_wrap) (just when it's going out of 
fashion again, apparently) so I'll never burn my fingers that way again. But the power, ease of use and 
flexibility of ARM microcontrollers these days blows me away. I'd been hearing all the Arduino buzz but 
hadn't realized how far we've come from those 8-bit PICs I burned up back in high school. And yeah I'm sure
Arduinos are cool, but the [Teensy](https://www.pjrc.com/teensy/) is even cooler because it can register
as a class-compliant USB MIDI device, making it super easy to achieve plug-and-play MIDI input.
