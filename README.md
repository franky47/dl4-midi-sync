# Line6 DL4 MIDI Clock Sync

Arduino sketch to automate the Tap Tempo switch of a Line6 DL4 delay to
sync it with MIDI Clock.

## Features

- Follows MIDI clock and syncs the DL4 delay time to it
- [Tempo subdivisions](#subdivisions), controlled via:
  - The Delay Time knob
  - MIDI CC
- Enable/Disable sync via MIDI CC
- Self-disables on Looper mode
- Configurable MIDI channel (for CCs)

## How It Works

In essence, the Arduino taps the tempo for you. Three taps, at the right
interval (landing on MIDI clock events), following tempo changes and
allowing you to select tempo subdivisions.

## Subdivisions

The following tempo subdivisions are supported <small>_(T: triplet, •: dotted)_</small>:

- 1/32
- 1/32T
- 1/16
- 1/16T
- 1/16•
- 1/8
- 1/8T
- 1/8•
- 1/4
- 1/4T
- 1/4•
- 1/2
- 1/2T

Larger subdivisions could be supported, but can conflict with the maximum delay length of the DL4, and varies from one delay model to the next.

## Inputs

The
[Arduino MIDI Library](https://github.com/fortyseveneffects/arduino_midi_library)
is used for the MIDI input.

The four pins of the delay mode encoder are read to disable the sync when
the DL4 is set to Looper mode (where the Tap Tempo switch has another function). The encoder is also used to set the MIDI channel in setup mode.

MIDI CC 10 is used for tempo subdivision
MIDI CC 74 is used to enable/disable the sync

The Delay Time potentiometer is read to control the tempo subdivision.
Not all values are available, only 9 have been selected to land on the 9
marking dots around the knob. All subdivisions are available through MIDI CC.

## Outputs

No MIDI out is sent, the internal delay time cannot be used to send MIDI clock
signals to other MIDI devices, it works the other way around.

The main "output" is the remote actuation of the Tap Tempo switch, by
toggling a pin between Input (Hi-Z) and Output Low for short pulses.

## Setting the MIDI input channel

The MIDI input channel to listen to for ControlChange messages can be set with
the following operation:

1. Power off the DL4
2. Set the Delay Mode encoder to the MIDI channel you want to listen to:

- Looper is channel 1
- Tube Echo is channel 2
- ...
- Auto-volume Echo is channel 16

3. Press the Tap Tempo switch, and **while it's pressed**, power on the DL4
4. Wait 3 seconds
5. Release the Tap Tempo switch: the MIDI channel is now saved.

## FAQ

#### Can this work with {other delay model} ?

You would need to look into how the Tap Tempo switch works on that unit.
If it shortens a normally-high level to the ground when pressed, then probably.

#### Can I sync my {MIDI unit} to the delay's internal clock ?

No. This works the other way around, the delay syncs to an incoming clock.

#### Can we go further ?

Probably, some ideas:

- Using ProgramChange to remote-control the Delay Mode encoder. When set to
  Looper, all encoder pins are in an idle state, and the Arduino could take over
  and change the delay mode when receiving ProgramChange messages.
