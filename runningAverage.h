#pragma once

template<unsigned SizeBits, typename T, typename SumType = long long>
struct RunningAverage
{
public:
  static const unsigned sSize = 1 << SizeBits;
  static const unsigned sMask = sSize - 1;

public:
  RunningAverage()
    : index(0)
    , usage(0)
  {
    memset(buffer, 0, sizeof(T) * sSize);
  }

public:
  inline void reset()
  {
    memset(buffer, 0, sizeof(T) * sSize);
    index = 0;
    usage = 0;
  }

  inline void push(T input)
  {
    buffer[index] = input;
    index = (index + 1) & sMask;
    usage = min(usage + 1, sSize);
  }

  inline T average() const
  {
    if (usage == 0)
    {
      return 0;
    }
    SumType sum = 0;
    for (unsigned i = 0; i < usage; ++i)
    {
      sum += buffer[i];
    }
    return sum / (SumType)(usage);
  }

  inline unsigned samples() const
  {
    return usage;
  }

private:
  T buffer[sSize];
  unsigned index;
  unsigned usage;
};
