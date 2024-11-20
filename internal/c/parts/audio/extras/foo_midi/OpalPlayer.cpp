//----------------------------------------------------------------------------------------------------------------------
// foo_midi: A foobar2000 component to play MIDI files (https://github.com/stuerp/foo_midi)
// Copyright (c) 2022-2024 Peter Stuer
//
// Modified by a740g for QB64-PE
//----------------------------------------------------------------------------------------------------------------------

#include "OpalPlayer.h"

OpalPlayer::OpalPlayer(InstrumentBankManager *ibm) : MIDIPlayer(), instrumentBankManager(ibm), synth(nullptr) {}

OpalPlayer::~OpalPlayer() {
    Shutdown();
}

bool OpalPlayer::Startup() {
    if (_IsInitialized)
        return true;

    if (!instrumentBankManager || instrumentBankManager->GetType() != InstrumentBankManager::Type::Opal)
        return false;

    synth = new OPLPlayer(chipCount, _SampleRate);
    if (synth) {
        if (instrumentBankManager->GetLocation() == InstrumentBankManager::Location::File) {
            if (!synth->loadPatches(instrumentBankManager->GetPath())) {
                delete synth;
                synth = nullptr;

                return false;
            }
        } else {
            if (!synth->loadPatches(instrumentBankManager->GetData(), instrumentBankManager->GetDataSize())) {
                delete synth;
                synth = nullptr;

                return false;
            }
        }

        _MIDIFlavor = MIDIFlavor::None;
        _FilterEffects = false;
        _IsInitialized = true;

        return true;
    }

    return false;
}

void OpalPlayer::Shutdown() {
    delete synth;
    synth = nullptr;
    _IsInitialized = false;
}

uint32_t OpalPlayer::GetActiveVoiceCount() const {
    return synth->activeVoiceCount();
}

void OpalPlayer::Render(audio_sample *buffer, uint32_t frames) {
    synth->generate(buffer, frames);
}

void OpalPlayer::SendEvent(uint32_t data) {
    auto channel = uint8_t(data & 0x0F);
    auto command = uint8_t(data & 0xF0);
    auto param1 = uint8_t((data >> 8) & 0x7F);
    auto param2 = uint8_t((data >> 16) & 0x7F);

    switch (command) {
    case StatusCodes::NoteOff:
        synth->midiNoteOff(channel, param1);
        break;

    case StatusCodes::NoteOn:
        synth->midiNoteOn(channel, param1, param2);
        break;

    case StatusCodes::ControlChange:
        synth->midiControlChange(channel, param1, param2);
        break;

    case StatusCodes::ProgramChange:
        synth->midiProgramChange(channel, param1);
        break;

    case StatusCodes::PitchBendChange:
        synth->midiPitchControl(channel, double((int16_t)(param1 | (param2 << 7)) - 8192) / 8192.0);
        break;
    }
}

void OpalPlayer::SendSysEx(const uint8_t *event, size_t size, uint32_t portNumber) {
    synth->midiSysEx(event, size);
}
