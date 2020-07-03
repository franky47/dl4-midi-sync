#pragma once

template<unsigned Pin, unsigned long Duration = 100>
struct PulseGenerator
{
public:
  PulseGenerator()
    : pulseStartTime(0)
  {
  }

  inline void setup()
  {
    pinMode(LED_BUILTIN, OUTPUT);
    reset();
  }

  inline void reset()
  {
    pinMode(Pin, INPUT);
    digitalWrite(LED_BUILTIN, LOW);
    pulseStartTime = 0;
  }

  inline void trigger()
  {
    pinMode(Pin, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    pulseStartTime = millis();
  }

  inline void tick()
  {
    if (pulseStartTime == 0) {
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
