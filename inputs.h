#pragma once

template<unsigned PinA, unsigned PinB, unsigned PinC, unsigned PinD>
struct ModeEncoder
{
public:
  static inline void setup()
  {
    pinMode(PinA, INPUT);
    pinMode(PinB, INPUT);
    pinMode(PinC, INPUT);
    pinMode(PinD, INPUT);
  }

  static inline byte read()
  {
    const byte a = digitalRead(PinA);
    const byte b = digitalRead(PinB);
    const byte c = digitalRead(PinC);
    const byte d = digitalRead(PinD);
    return a << 3 | b << 2 | c << 1 | d;
  }
};

enum DelayModes
{
  looper = 0b1111,
  tubeEcho = 0b0111,
  tapeEcho = 0b0011,
  multiHead = 0b1011,
  sweepEcho = 0b1001,
  analogEcho = 0b0001,
  analogWithMod = 0b0101,
  loResDelay = 0b1101,
  digitalDelay = 0b1100,
  digitalWithMod = 0b0100,
  rythmicDelay = 0b0000,
  stereoDelays = 0b1000,
  pingPong = 0b1010,
  reverse = 0b0010,
  dynamicDelay = 0b0110,
  autoVolumeEcho = 0b1110,
};

// --

template<unsigned Pin, unsigned NumSegments>
struct AnalogSegments
{
public:
  void setup()
  {
    // The ADC runs on 10 bits, with 2 bits of hysteresis we can only support
    //  maximum of 256 steps.
    static_assert(NumSegments <= 256, "Only up to 256 segments are supported.");
    pinMode(Pin, INPUT);
    delay(10);
    previousOutput = map(analogRead(Pin));
  }

  unsigned read()
  {
    static const int segmentSize = 1024 / NumSegments;

    const int value = analogRead(Pin);
    const int segment = map(value);
    const int segmentStart = segment * segmentSize;
    const int segmentEnd = (segment + 1) * segmentSize - 1;
    const int distanceFromStart = value - segmentStart;
    const int distanceFromEnd = segmentEnd - value;
    if (min(distanceFromStart, distanceFromEnd) <= 4)
    {
      return previousOutput;
    }
    previousOutput = segment;
    return segment;
  }

protected:
  static inline int map(unsigned inValue)
  {
    return inValue * NumSegments >> 10;
  }

private:
  int previousOutput;
};

// --

template<typename T>
struct ChangeListener
{
  using Listener = void (*)(T current);

public:
  void setup(Listener inListener, T startupValue = T(0))
  {
    mListener = inListener;
  }

  void read(T inCurrentValue)
  {
    if (inCurrentValue == mLastValue)
    {
      return;
    }
    mListener(inCurrentValue);
    mLastValue = inCurrentValue;
  }

private:
  Listener mListener;
  T mLastValue;
};
