//----------------------------------------------------------------------------------------------------------------------
// foo_midi: A foobar2000 component to play MIDI files (https://github.com/stuerp/foo_midi)
// Copyright (c) 2022-2024 Peter Stuer
//
// Modified by a740g for QB64-PE
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "../ymfmidi/player.h"
#include "InstrumentBankManager.h"
#include "MIDIPlayer.h"

class OpalPlayer : public MIDIPlayer {
  public:
    OpalPlayer() = delete;
    OpalPlayer(const OpalPlayer &) = delete;
    OpalPlayer(OpalPlayer &&) = delete;
    OpalPlayer &operator=(const OpalPlayer &) = delete;
    OpalPlayer &operator=(OpalPlayer &&) = delete;

    OpalPlayer(InstrumentBankManager *ibm);
    virtual ~OpalPlayer();

    virtual uint32_t GetActiveVoiceCount() const override;

  protected:
    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample *buffer, uint32_t frames) override;

    virtual void SendEvent(uint32_t data) override;
    virtual void SendSysEx(const uint8_t *event, size_t size, uint32_t portNumber) override;

  private:
    static constexpr unsigned chipCount = 6; // each OPL3 chip has 18 voices; this value is per recommendation in DMXOPL3

    InstrumentBankManager *instrumentBankManager;
    OPLPlayer *synth;
};
