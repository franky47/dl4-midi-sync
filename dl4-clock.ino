#include <MIDI.h>
#include <EEPROMex.h>
#include "./pulseGenerator.h"
#include "./clock.h"
#include "./inputs.h"
#include "./settings.h"

MIDI_CREATE_DEFAULT_INSTANCE();

// Pin mapping --

static const byte tapTempoPin     = 8;
static const byte delayLengthPin  = A0;
static const byte modeEncoderPin1 = A1;
static const byte modeEncoderPin2 = A2;
static const byte modeEncoderPin3 = A3;
static const byte modeEncoderPin4 = A4;

// Control Change mapping --

static const byte ccTimeDivision  = 10;
static const byte ccSyncEnable    = 74;

// Global objects --

Settings settings;
Clock clock;
PulseGenerator<tapTempoPin, 100> tapTempoPulse;
AnalogSegments<delayLengthPin, 9> divisionsPot;
ModeEncoder<
  modeEncoderPin1,
  modeEncoderPin2,
  modeEncoderPin3,
  modeEncoderPin4
> modeEncoder;

// --

// Declarations to be used by the controller
void onClock();

struct Controller
{
public:
  static inline void setup()
  {
    isInLooperMode = modeEncoder.read() == DelayModes::looper;
    isSyncEnabledByCC = true;
    updateState();
  }

  static inline void onDelayModeChange(byte mode)
  {
    isInLooperMode = mode == DelayModes::looper;
    updateState();
  }
  static inline void onSyncEnableCC(bool enableSync)
  {
    isSyncEnabledByCC = enableSync;
    updateState();
  }

private:
  static inline void updateState()
  {
    const bool enableClockCallback = !isInLooperMode && isSyncEnabledByCC;
    MIDI.setHandleClock(enableClockCallback ? onClock : nullptr);
  }

private:
  static bool isInLooperMode;
  static bool isSyncEnabledByCC;
};

bool Controller::isInLooperMode;
bool Controller::isSyncEnabledByCC;

// --

static const DivisionLengths divisionPotMapping[9] = {
  DivisionLengths::regular32nd,
  DivisionLengths::regular16th,
  DivisionLengths::dotted16th,
  DivisionLengths::regular8th,
  DivisionLengths::dotted8th,
  DivisionLengths::regularQuarter,
  DivisionLengths::dottedQuarter,
  DivisionLengths::triplet8th,
  DivisionLengths::tripletQuarter,
};

static const DivisionLengths ccDivisionMapping[13] = {
                                      /* CC Values: */
  DivisionLengths::regular32nd,       /* 000 to 009 */
  DivisionLengths::regular16th,       /* 010 to 019 */
  DivisionLengths::regular8th,        /* 020 to 029 */
  DivisionLengths::regularQuarter,    /* 030 to 039 */
  DivisionLengths::regularHalf,       /* 040 to 049 */

  DivisionLengths::dotted16th,        /* 050 to 059 */
  DivisionLengths::dotted8th,         /* 060 to 069 */
  DivisionLengths::dottedQuarter,     /* 070 to 079 */

  DivisionLengths::triplet32nd,       /* 080 to 089 */
  DivisionLengths::triplet16th,       /* 090 to 099 */
  DivisionLengths::triplet8th,        /* 100 to 109 */
  DivisionLengths::tripletQuarter,    /* 110 to 119 */
  DivisionLengths::tripletHalf,       /* 120 to 127 */
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
  const bool sendTapTempoPulse = clock.tick();
  if (sendTapTempoPulse)
  {
    tapTempoPulse.trigger();
  }
}

void onControlChange(byte channel, byte control, byte value)
{
  if (control == ccTimeDivision)
  {
    const byte division = ccDivisionMapping[value / 10];
    clock.setDivision(division);
  }
  if (control == ccSyncEnable)
  {
    const bool enableSync = value >= 64;
    Controller::onSyncEnableCC(enableSync);
  }
}

// Change listeners --

void onDivisionPotChange(byte index)
{
  clock.setDivision(divisionPotMapping[index]);
}

// --

struct SetupMode
{
  static inline void run()
  {
    if (digitalRead(tapTempoPin) != LOW)
    {
      return;
    }

    const byte encoder = modeEncoder.read();

    // The MIDI channel is encoded clockwise from the Looper position,
    // but the encoder's Gray code does not have a trivial mapping.
    // We could use an algorithm to convert back to binary, but
    // a switch case better showcases the link between channel and position.
    switch (encoder)
    {
      default:
      case DelayModes::looper:
        settings.midiChannel = 1;
        break;
      case DelayModes::tubeEcho:
        settings.midiChannel = 2;
        break;
      case DelayModes::tapeEcho:
        settings.midiChannel = 3;
        break;
      case DelayModes::multiHead:
        settings.midiChannel = 4;
        break;
      case DelayModes::sweepEcho:
        settings.midiChannel = 5;
        break;
      case DelayModes::analogEcho:
        settings.midiChannel = 6;
        break;
      case DelayModes::analogWithMod:
        settings.midiChannel = 7;
        break;
      case DelayModes::loResDelay:
        settings.midiChannel = 8;
        break;
      case DelayModes::digitalDelay:
        settings.midiChannel = 9;
        break;
      case DelayModes::digitalWithMod:
        settings.midiChannel = 10;
        break;
      case DelayModes::rythmicDelay:
        settings.midiChannel = 11;
        break;
      case DelayModes::stereoDelays:
        settings.midiChannel = 12;
        break;
      case DelayModes::pingPong:
        settings.midiChannel = 13;
        break;
      case DelayModes::reverse:
        settings.midiChannel = 14;
        break;
      case DelayModes::dynamicDelay:
        settings.midiChannel = 15;
        break;
      case DelayModes::autoVolumeEcho:
        settings.midiChannel = 16;
        break;
    }
    settings.save();

    // Wait for the Tap Tempo pin to be released
    while (digitalRead(tapTempoPin) == LOW)
    {
      delay(10);
    }
  }
};

// --

void setup()
{
  // IO Setup
  tapTempoPulse.setup();
  modeEncoder.setup();
  divisionsPot.setup();
  SetupMode::run();
  settings.load();

  MIDI.begin(settings.midiChannel);
  Controller::setup();
  MIDI.setHandleControlChange(onControlChange);
  MIDI.setHandleStart(onStart);
  MIDI.turnThruOff();
  clock.setup();

  // Listeners
  divisionPotListener.setup(onDivisionPotChange, divisionsPot.read());
  delayModeListener.setup(Controller::onDelayModeChange, modeEncoder.read());
}

void loop()
{
  divisionPotListener.read(divisionsPot.read());
  delayModeListener.read(modeEncoder.read());
  MIDI.read();
  tapTempoPulse.tick();
}
