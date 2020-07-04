#pragma once
#include <inttypes.h>

enum SettingsAddresses
{
  midiChannel = 0x0000
};

struct Settings
{
  uint8_t midiChannel;

  inline void load()
  {
    midiChannel = EEPROM.readByte(SettingsAddresses::midiChannel);
  }
  inline void save()
  {
    EEPROM.updateByte(SettingsAddresses::midiChannel, midiChannel);
  }
};
