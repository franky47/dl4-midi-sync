# Adding MIDI Clock to a Line6 DL4 Delay

This post is a bit different from what I usually write about. My job as a
[freelance software engineer](https://francoisbest.com)
often leads me to write about web develoment, front-end
design and general software things, but this post is about another passion of
mine: making music. Or rather, working on music gear.

I'm in the process of remaking my home studio, moving from the coldness of
computers, VSTs and software instruments to the warmth of hardware synths.

Synthesizers sound better with effects, and the right guitar pedal can turn a
cheap synth into a massive soundscaping beast. Delays are especially effective
in bringing depth to an arpeggiated sequence.

However, delay pedals aren't often designed with a digital setup in mind, and
don't always come with a MIDI input that could sync the delay time to the
tempo of the song.

This is the case for my Line6 DL4, a great sounding piece of gear, but limited
in its interaction with the rest of my setup.

I set out to give it some MIDI capabilities, this post is about how it was done,
using an Arduino board.

> **Disclaimer**
> The modifications done here will void your warranty, and I cannot be held
> responsible for damages to either yourself, others or your gear.

## Hardware Analysis

First, we need to know what we can work with. The DL4 has a Tap Tempo footswitch,
that we could probably "automate". That's the general idea behind this project.

Next, we'll want to control which subdivisions of the tempo the delay is
going to sync to. This could be quarter notes, eights, dotted, triplets..
We'll come back to that in a moment.

Finally, the DL4 has a looper mode, where the Tap Tempo switch is used for
something else (changing playback speed and reversing it). We will want our
MIDI sync to disable itself when the unit is set to Looper mode.

## Hacking The switch

The DL4 reads its switches using a voltage divider. This technique is often used
to save some inputs on a microcontroller: rather than having one input per switch,
where every switch has a boolean on/off state, each switch is placed in the analog
equivalent of a bit field, and outputs an analog voltage that the controller can
read to know which switch(es) were pressed.

There are some limitations to this technique though: if you press more than
two switches at once, you will lose some information. But with carefully selected
resistor values, it's possible to detect up to two switches pressed at the same
time.

In our case, we're only interested in controlling the Tap Tempo switch. It
connects R32 to the ground when pressed, and leaves it hanging when released.
Let's see how we can interface that with an Arduino.

Arduino pins can have four states:

- Input (Hi-Z)
- Input (with pull-up)
- Output Low
- Output High

A Hi-Z (high impedance) input pin does not draw current nor sink it,
it's _transparent_ to the circuit. Output Low is equivalent to connecting to ground.

This is exactly what we need: by default the pin is set to Input Hi-Z, and
does not do anything, and when we want to "press" the switch, we change the
pin to Output Low, which will connect R32 to the ground via the Arduino.

As it turns out, it's very easy to do with Arduino code:

```cpp
// When idle:
pinMode(tapTempoPin, INPUT);

// When pressed:
pinMode(tapTempoPin, OUTPUT);
```

## Generating Pulses

We can't really turn our pin into an output then immediately turn it back into
an input and call it a day. The DL4 expects a human to tap the tempo, and even
the fastest human can't tap as fast as a 20MHz microcontroller.

We need to slow the pulse down.

We could do it this way:

```cpp
pinMode(tapTempoPin, OUTPUT);
delay(100); // Wait 100ms
pinMode(tapTempoPin, INPUT);
```

However, using `delay` comes with a drawback: while the Arduino is waiting,
it's not doing anything else, like counting MIDI `Clock` messages.

Active waiting, also called "blocking the main thread", is something
to avoid in general to keep programs responsive.

Instead, we can build a little piece of code that will trigger the pulse,
keep a record of how long it's been since the pulse started, and reset it
after enough time has passed, but doing so with minimal blocking, allowing
other pieces of code to run.

This is a concurrent approach to software design, as Arduino (like JavaScript)
only has a single thread of operations. If you want to do something later
without blocking the rest of the program, you have to schedule it.

Let's see how our pulse generator could look like:

```cpp
// pulseGenerator.h
#pragma once

template<unsigned Pin, unsigned long Duration = 100>
struct PulseGenerator
{
public:
  PulseGenerator()
    : pulseStartTime(0)
  {
  }

  void reset()
  {
    pinMode(Pin, INPUT);
    pulseStartTime = 0;
  }

  void trigger()
  {
    pinMode(Pin, OUTPUT);
    pulseStartTime = millis();
  }

  void tick()
  {
    if (pulseStartTime == 0)
    {
      return;
    }
    const unsigned long now = millis();
    if ((now - pulseStartTime) >= Duration)
    {
      reset();
    }
  }

private:
  unsigned long pulseStartTime;
};
```

We're using C++ templates here for two things:

- The hardware pin we're going to control
- The duration of the pulse in milliseconds

I considered that these were not parameters we want to change at runtime,
so passing them as template parameters lets the compiler optimise them away.

The only state we keep is the time at which the pulse started, which has two uses:

- Tell whether we're currently pulsing or not
- Count how much time has passed

Now that we have a way to remotely tap the tempo, we need to synchronise it
with the MIDI clock.

## Divide To Conquer: MIDI Time Divisions

To synchronise to the tempo of the song, we'll listen to MIDI `Clock` events.

Those are sent 24 times per quarter note, when sequencers are playing.

This means that if we wait for every 24th `Clock` messages before pressing on
the Tap Tempo switch, we'll sync our delay to quarter notes.

We can also choose to wait for fewer `Clock` messages to sync to a faster
rythm, of more for a slower rythm.

```
// Regular:
// ••• 1/32nd (3 Clocks)
// •••••• 1/16th (6 Clocks)
// ••••••••• Dotted 1/16 (9 Clocks)
// •••••••••••• 1/8 (12 Clocks)
// •••••••••••••••••• Dotted 1/8 (18 Clocks)
// •••••••••••••••••••••••• 1/4 (24 Clocks)
// •••••••••••••••••••••••••••••••••••• Dotted 1/4 (36 Clocks)
// •••••••••••••••••••••••••••••••••••••••••••••••• 1/2 (48 Clocks)

// Triplets:
// •• 1/32T (2 Clocks)
// •••• 1/16T (4 Clocks)
// •••••••• 1/8T (8 Clocks)
// •••••••••••••••• 1/4T (16 Clocks)
// •••••••••••••••••••••••••••••••• 1/2T (32 Clocks)
```

## Controlling the Subdivisions

We can now set the subdivisions, but we need to a way to control them. There are
multiple ways we can do it:

- Manually, on the front panel
- Via MIDI, using a Control Change

On the front panel, it makes most sense to try and reuse the Delay Time knob,
as it's the one controlling the continuous delay time when the unit is not
synced to the MIDI clock.

This is what most VSTs and hardware units do: continuous time control when
using the internal clock, and subdivisions of the external clock when synced.

However, we can't easily disconnect the knob from its primary function, so here
again, we're going to have to act in parallel of the unit's default behaviour.

It's not a big issue: when moving the knob, the delay will attempt to set its
own internal time, but the tap tempo automation will move it back into sync
after a few pulses. It might make for interesting pitching effects though.

We can read the central pin of the Speed potentiometer (R24) and feed that
to one of Arduino's analog input pins.

Analog inputs are read with 10 bits of precision, however we only need 16
subdivisions (4 bits), so we could shift the reading by 6.
However, analog inputs can be noisy, and if the pot is at the edge between two
subdivisions, it could easily bounce between one or the other. As there will be
no markings, it would be hard to precisely center the pot on a subdivision,
so we can use hysteresis thresholds instead.

An alternative I ended up going for was to limit the number of subdivisions
to the number of marks on the Speed potentiometer (9), so that it's easier to
hit the right one. Plus some of the subdivisions aren't very musical, and don't
need an easy access (they can still be accessed via Control Change, read on).

### Control Change

We can easily use a Control Change (CC) message to control the subdivision.

We can also use a CC to enable or disable the Sync effect. This one would map
nicely to CC 76, which is an on/off switch for delay effects:

```cpp{6,7,8,9,10}
static const byte ccSyncEnable = 76;

void onControlChange(byte channel, byte control, byte value)
{
  // ...
  if (control == ccSyncEnable)
  {
    const bool enabled = value >= 64;
    MIDI.setHandleClock(enabled ? onClock : nullptr);
  }
  // ...
}
```

This piece of code registers or un-registers the
[MIDI callback](https://github.com/FortySevenEffects/arduino_midi_library/wiki/Using-Callbacks)
for `Clock` events.

## Looper mode

When the DL4 is set to Looper mode, the Tap Tempo switch no longer acts as such,
and therefore our program should stop trying to remotely press the switch.

We can detect the mode of the unit by reading the mode selector switch,
which encodes the 16 positions using four internal switches.

Because those switches connect to the ground when active or leave the pin
floating when idle, we can assume the target chip P80C32SBBB uses its internal
pull-ups to read those values.

The switch encoding is as follows:

```
(as seen from the back: GND A B C D)
0: no connection
1: connection

0000 => Looper
1000 => Tube Echo
1100 => Tape Echo
0100 => Multi-Head
0110 => Sweep Echo
1110 => Analog Echo
1010 => Analog w/ mod
0010 => Lo-res Delay
0011 => Digital Delay
1011 => Digital w/ mod
1111 => Rythmic Delay
0111 => Stereo Delays
0101 => Ping Pong
1101 => Reverse
1001 => Dynamic Delay
0001 => Auto-Volume Echo
```

> The encoding may seem weird and not following binary progression. This is
> [Gray code](https://en.wikipedia.org/wiki/Gray_code), it's a common
> thing found in encoders, to have only a single bit change from one position to
> the next.
> <br />
> It's mirrored because I did the measurement on the back of the unit.

Since the encoder connects a pull-up resistor to the ground, and we're going
to measure levels, we need to invert those values:

```
(as seen from the back: GND A B C D)
L: LOW level readout
H: HIGH level readout

HHHH => Looper
LHHH => Tube Echo
LLHH => Tape Echo
HLHH => Multi-Head
HLLH => Sweep Echo
LLLH => Analog Echo
LHLH => Analog w/ mod
HHLH => Lo-res Delay
HHLL => Digital Delay
LHLL => Digital w/ mod
LLLL => Rythmic Delay
HLLL => Stereo Delays
HLHL => Ping Pong
LLHL => Reverse
LHHL => Dynamic Delay
HHHL => Auto-Volume Echo
```

Looper being encoded with `HHHH` is a good thing, it will simplify our code:

```cpp
const byte a = digitalRead(PinA);
const byte b = digitalRead(PinB);
const byte c = digitalRead(PinC);
const byte d = digitalRead(PinD);
const bool isLooper = (a & b & c & d) == 1;
```

## Additional Features

### Tap count

If the tempo is stable and nothing changed, we stop tapping after 4 taps,
otherwise some glitches can occur.

### MIDI channel

There are 16 position on the mode selector, which is ideal to setup the MIDI
channel to use for Control Changes.

We can save the MIDI channel setting in EEPROM, to be recalled on startup.

To setup the MIDI channel, we can detect the state of the Tap Tempo switch at
startup: if it's pressed, we read the positon of the mode selector and infer the
MIDI channel, then wait for Tap Tempo to be released to resume normal operations.

---

## Raw notes

There is a maximum delay time over which the unit stops understanding tap tempo,
unfortunately it depends on the delay type. Fortunately, we're going to read it,
so we could implement a workaround, and sync to the closest division.
But that means we have to measure tempo as well as counting Clock messages.

### Secret startup mode

> Or _"mess around with buttons and discover weird behaviours"_

- Hold switch A => very weird interaction with switches and knobs, the audio
  plays a single delay (dump of the delay line buffer ?), maybe a factory test mode ?
- Hold Tap Tempo => LED 1 flashes once, then 2 flashes 3 times. Could indicate firmware revision number 1.3 ?
