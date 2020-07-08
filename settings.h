#pragma once

enum SettingsAddresses
{
  midiChannel = 0x0000
};

struct Settings
{
  byte midiChannel;

  inline void load()
  {
    midiChannel = EEPROM.readByte(SettingsAddresses::midiChannel);
    if (midiChannel == 0 || midiChannel > 16)
    {
      midiChannel = 1;
    }
  }
  inline void save()
  {
    EEPROM.updateByte(SettingsAddresses::midiChannel, midiChannel);
  }
};
