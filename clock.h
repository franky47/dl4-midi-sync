#pragma once
#include <inttypes.h>
#include "./runningAverage.h"

enum DivisionLengths {
  regular32nd = 3,
  regular16th = 6,
  regular8th = 12,
  regularQuarter = 24,
  regularHalf = 48,
  dotted16th = 9,
  dotted8th = 18,
  dottedQuarter = 36,
  triplet32nd = 2,
  triplet16th = 4,
  triplet8th = 8,
  tripletQuarter = 16,
  tripletHalf = 32
};

// --

struct Clock
{
public:
  inline void setup()
  {
    reset();
    clockCycle = DivisionLengths::dotted8th;
    Serial.begin(115200);
    while (!Serial) {
      delay(10);
    }
  }

  inline void reset()
  {
    clockCounter = 0;
    pulseCounter = 0;
    lastTickTimestamp = 0;
    tickDurationShortAvg.reset();
    tickDurationLongAvg.reset();
  }

  inline bool tick()
  {
    const long now = micros();
    if (lastTickTimestamp == 0)
    {
      // Initial tick
      lastTickTimestamp = now;
      Serial.print("\tinitial tick ");
      return true;
    }
    if (tickDurationShortAvg.samples() == 0)
    {
      // Second tick
      const long tickDuration = now - lastTickTimestamp;
      tickDurationShortAvg.push(tickDuration);
      tickDurationLongAvg.push(tickDuration);
      incrementClockCounter();
      Serial.print("\tsecond tick ");
      return shouldSendPulse();
    }
    // Subsequent ticks
    incrementClockCounter();
    const bool tempoChanged = checkForTempoChange(now);
    if (tempoChanged)
    {
      // Start tapping again at the next clock cycle.
      pulseCounter = 0;
    }
    Serial.print("\tcc: ");
    Serial.print(clockCounter);
    Serial.print("\ttc: ");
    Serial.print(tempoChanged);
    return (clockCounter == 0 || tempoChanged) ? shouldSendPulse() : false;
  }

  inline void setDivision(DivisionLengths inDivisionLength)
  {
    Serial.print("div: ");
    Serial.println(inDivisionLength);
    clockCycle = inDivisionLength;
    pulseCounter = 0;
    clockCounter = clockCycle; // Insures we get a pulse at the next tick
  }

private:
  inline void incrementClockCounter()
  {
    clockCounter++;
    if (clockCounter >= clockCycle)
    {
      clockCounter = 0;
    }
  }

  inline bool checkForTempoChange(long now)
  {
    const long tickDuration = now - lastTickTimestamp;
    Serial.print("\ttick: ");
    Serial.print(tickDuration);
    lastTickTimestamp = now;

    if (tickDuration < 0)
    {
      // Timestamp overflow, ignore
      return false;
    }
    tickDurationShortAvg.push(tickDuration);
    tickDurationLongAvg.push(tickDuration);
    const long avgTickDurationShort = tickDurationShortAvg.average();
    const long avgTickDurationLong = tickDurationLongAvg.average();
    const long delta = abs(avgTickDurationLong - avgTickDurationShort);
    Serial.print("\tdelta: ");
    Serial.print(delta);
    const long deltaPct = avgTickDurationShort == 0
      ? 100
      : 100 * delta / avgTickDurationShort;
    Serial.print("\tpct: ");
    Serial.print(deltaPct);
    return deltaPct > 0;
  }

  inline bool shouldSendPulse()
  {
    if (clockCounter != 0)
    {
      return false;
    }
    if (pulseCounter < 3)
    {
      pulseCounter++;
      return true;
    }
    return false;
  }

private:
  uint8_t clockCounter;
  uint8_t pulseCounter;
  uint8_t clockCycle;
  long lastTickTimestamp;
  RunningAverage<2, long> tickDurationShortAvg;
  RunningAverage<4, long> tickDurationLongAvg;
};
