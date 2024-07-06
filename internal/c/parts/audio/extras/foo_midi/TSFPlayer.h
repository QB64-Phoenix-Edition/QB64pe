//----------------------------------------------------------------------------------------------------------------------
// foo_midi: A foobar2000 component to play MIDI files (https://github.com/stuerp/foo_midi)
// Copyright (c) 2022-2024 Peter Stuer
//
// Modified by a740g for QB64-PE
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "../tinysoundfont/tsf.h"
#include "InstrumentBankManager.h"
#include "MIDIPlayer.h"

class TSFPlayer : public MIDIPlayer {
  public:
    TSFPlayer() = delete;
    TSFPlayer(const TSFPlayer &) = delete;
    TSFPlayer(TSFPlayer &&) = delete;
    TSFPlayer &operator=(const TSFPlayer &) = delete;
    TSFPlayer &operator=(TSFPlayer &&) = delete;

    TSFPlayer(InstrumentBankManager *ibm);
    virtual ~TSFPlayer();

    virtual uint32_t GetActiveVoiceCount() const override;

  protected:
    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample *buffer, uint32_t frames) override;

    virtual void SendEvent(uint32_t data) override;

  private:
    static const uint32_t renderFrames = 64;

    InstrumentBankManager *instrumentBankManager;
    tsf *synth;
};
