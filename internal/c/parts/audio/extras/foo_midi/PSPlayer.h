//----------------------------------------------------------------------------------------------------------------------
// foo_midi: A foobar2000 component to play MIDI files (https://github.com/stuerp/foo_midi)
// Copyright (c) 2022-2024 Peter Stuer
//
// Modified by a740g for QB64-PE
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "../primesynth/primesynth.h"
#include "InstrumentBankManager.h"
#include "MIDIPlayer.h"

class PSPlayer : public MIDIPlayer {
  public:
    PSPlayer() = delete;
    PSPlayer(const PSPlayer &) = delete;
    PSPlayer(PSPlayer &&) = delete;
    PSPlayer &operator=(const PSPlayer &) = delete;
    PSPlayer &operator=(PSPlayer &&) = delete;

    PSPlayer(InstrumentBankManager *ibm);
    virtual ~PSPlayer();

    virtual uint32_t GetActiveVoiceCount() const override;

  protected:
    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample *buffer, uint32_t frames) override;

    virtual void SendEvent(uint32_t data) override;
    virtual void SendSysEx(const uint8_t *event, size_t size, uint32_t portNumber) override;

  private:
    InstrumentBankManager *instrumentBankManager;
    primesynth::Synthesizer *synth;
};
