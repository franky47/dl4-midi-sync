#pragma once
#include <inttypes.h>
#include "./runningAverage.h"

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

enum DivisionLengths {
  regular32nd = 3,
  regular16th = 6,
  regularDotted16th = 9,
  regular8th = 12,
  regularDotted8th = 18,
  regularQuarter = 24,
  regularDottedQuarter = 36,
  regularHalf = 48,
  triplet32nd = 2,
  triplet16th = 4,
  triplet8th = 8,
  tripletQuarter = 16,
  tripletHalf = 32
};

// 0 1 2 3 4 5 6 7 8 9 a b c
// • - - - • - - - • - - - • -
// 0 1 2 3 0 1 2 3 0 1 2 3 0 1


struct Clock
{
public:
  inline void setup()
  {
    reset();
    clockCycle = DivisionLengths::triplet16th;// DivisionLengths::regularQuarter;
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

      // // debug
      // const long avgTickDurationShort = tickDurationShortAvg.average();
      // const long avgTickDurationLong = tickDurationLongAvg.average();
      // Serial.print("\tavgS: ");
      // Serial.print(avgTickDurationShort);
      // Serial.print("\tavgL: ");
      // Serial.print(avgTickDurationLong);
      // // gubed

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
    // Serial.print("\tnow: ");
    // Serial.print(now);
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
    // Serial.print("\tavgS: ");
    // Serial.print(avgTickDurationShort);
    // Serial.print("\tavgL: ");
    // Serial.print(avgTickDurationLong);
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
