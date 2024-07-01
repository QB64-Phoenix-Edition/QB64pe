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
    static constexpr unsigned chipCount = 4; // each OPL3 chip has 18 voices, so this gives us a total of 72 voices

    InstrumentBankManager *instrumentBankManager;
    OPLPlayer *synth;
};
