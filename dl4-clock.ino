#include <USB-MIDI.h>
#include "./pulseGenerator.h"
#include "./clock.h"
#include "./inputs.h"

USBMIDI_CREATE_DEFAULT_INSTANCE();

// Pin mapping --

PulseGenerator<8, 100> tapTempoPulse;
AnalogSegments<A0, 9> divisionsPot;
ModeEncoder<A1, A2, A3, A4> modeEncoder;

// Control Change mapping --

static const byte ccTimeDivision = 10;
static const byte ccSyncEnable = 74;

// Global objects --

Clock clock;

static const DivisionLengths divisionPotMapping[9] = {
  DivisionLengths::regular32nd,
  DivisionLengths::regular16th,
  DivisionLengths::regularDotted16th,
  DivisionLengths::regular8th,
  DivisionLengths::regularDotted8th,
  DivisionLengths::regularQuarter,
  DivisionLengths::regularDottedQuarter,
  DivisionLengths::triplet8th,
  DivisionLengths::tripletQuarter,
};

static const DivisionLengths ccDivisionMapping[12] = {
  DivisionLengths::regular32nd,
  DivisionLengths::regular16th,
  DivisionLengths::regular8th,
  DivisionLengths::regularQuarter,
  DivisionLengths::regularHalf,

  DivisionLengths::regularDotted16th,
  DivisionLengths::regularDotted8th,
  DivisionLengths::regularDottedQuarter,

  DivisionLengths::triplet16th,
  DivisionLengths::triplet8th,
  DivisionLengths::tripletQuarter,
  DivisionLengths::tripletHalf,
};

ChangeListener<byte> divisionPotListener;
ChangeListener<byte> delayModeListener;

// MIDI Callbacks --

void onStart()
{
  clock.reset();
}

void onClock()
{
  Serial.print("Clock");
  const bool sendTapTempoPulse = clock.tick();
  if (sendTapTempoPulse)
  {
    Serial.print(" + Pulse ");
    tapTempoPulse.trigger();
  }
  Serial.println("");
}

void onControlChange(byte channel, byte control, byte value)
{
  if (control == ccTimeDivision)
  {
    const byte division = ccDivisionMapping[value / 10];
    Serial.print("CC Division");
    Serial.println(division);
    clock.setDivision(division);
  }
  if (control == ccSyncEnable)
  {
    const bool enabled = value >= 64;
    MIDI.setHandleClock(enabled ? onClock : nullptr);
    Serial.print("CC Sync Enable");
    Serial.println(enabled);
  }
}

// Change listeners --

void onDivisionPotChange(byte index)
{
  clock.setDivision(divisionPotMapping[index]);
}

void onDelayModeChange(byte mode)
{
  MIDI.setHandleClock(mode == DelayModes::looper ? nullptr : onClock);
}

// --

long tick = 0;

void setup()
{
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleClock(onClock);
  MIDI.setHandleControlChange(onControlChange);
  MIDI.setHandleStart(onStart);
  clock.setup();
  tapTempoPulse.setup();
  // modeEncoder.setup();
  divisionsPot.setup();

  // Listeners
  divisionPotListener.setup(onDivisionPotChange, divisionsPot.read());
  // delayModeListener.setup(onDelayModeChange, modeEncoder.read());

  // Power for the MIDI micro-shield
  // pinMode(2, OUTPUT);
  // pinMode(3, OUTPUT);
  // digitalWrite(2, HIGH);

  // Debug
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("Arduino Ready");
  tick = micros();
}

void loop()
{
  divisionPotListener.read(divisionsPot.read());
  // delayModeListener.read(modeEncoder.read());
  MIDI.read();
  tapTempoPulse.tick();

  // const long tock = micros();
  // Serial.print("Loop delay (us): ");
  // Serial.println(tock - tick);
  // Serial.flush();
  // tick = tock;
}
