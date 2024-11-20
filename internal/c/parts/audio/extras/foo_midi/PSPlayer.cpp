//----------------------------------------------------------------------------------------------------------------------
// foo_midi: A foobar2000 component to play MIDI files (https://github.com/stuerp/foo_midi)
// Copyright (c) 2022-2024 Peter Stuer
//
// Modified by a740g for QB64-PE
//----------------------------------------------------------------------------------------------------------------------

#include "PSPlayer.h"

PSPlayer::PSPlayer(InstrumentBankManager *ibm) : MIDIPlayer(), instrumentBankManager(ibm), synth(nullptr) {}

PSPlayer::~PSPlayer() {
    Shutdown();
}

uint32_t PSPlayer::GetActiveVoiceCount() const {
    try {
        return synth->getActiveVoiceCount();
    } catch (...) {
        return 0;
    }

    return 0;
}

bool PSPlayer::Startup() {
    if (_IsInitialized)
        return true;

    if (!instrumentBankManager || instrumentBankManager->GetType() != InstrumentBankManager::Type::Primesynth ||
        instrumentBankManager->GetLocation() == InstrumentBankManager::Location::Memory)
        return false;

    try {
        synth = new primesynth::Synthesizer(_SampleRate);
    } catch (...) {
        return false;
    }

    if (synth) {
        try {
            synth->loadSoundFont(instrumentBankManager->GetPath());
        } catch (...) {
            delete synth;
            synth = nullptr;

            return false;
        }

        _MIDIFlavor = MIDIFlavor::None;
        _FilterEffects = false;
        _IsInitialized = true;

        return true;
    }

    return false;
}

void PSPlayer::Shutdown() {
    delete synth;
    synth = nullptr;
    _IsInitialized = false;
}

void PSPlayer::Render(audio_sample *buffer, uint32_t frames) {
    try {
        synth->render_float(buffer, frames << 1);
    } catch (...) {
        return;
    }
}

void PSPlayer::SendEvent(uint32_t data) {
    auto channel = uint8_t(data & 0x0F);
    auto command = uint8_t(data & 0xF0);
    auto param1 = uint8_t((data >> 8) & 0x7F);
    auto param2 = uint8_t((data >> 16) & 0x7F);

    try {
        switch (command) {
        case StatusCodes::NoteOff:
            synth->processChannelMessage(primesynth::midi::MessageStatus::NoteOff, channel, param1);
            break;

        case StatusCodes::NoteOn:
            synth->processChannelMessage(primesynth::midi::MessageStatus::NoteOn, channel, param1, param2);
            break;

        case StatusCodes::KeyPressure:
            synth->processChannelMessage(primesynth::midi::MessageStatus::KeyPressure, channel, param1, param2);
            break;

        case StatusCodes::ControlChange:
            synth->processChannelMessage(primesynth::midi::MessageStatus::ControlChange, channel, param1, param2);
            break;

        case StatusCodes::ProgramChange:
            synth->processChannelMessage(primesynth::midi::MessageStatus::ProgramChange, channel, param1);
            break;

        case StatusCodes::ChannelPressure:
            synth->processChannelMessage(primesynth::midi::MessageStatus::ChannelPressure, channel, param1);
            break;

        case StatusCodes::PitchBendChange:
            synth->processChannelMessage(primesynth::midi::MessageStatus::PitchBend, channel, param1, param2);
            break;
        }
    } catch (...) {
        return;
    }
}

void PSPlayer::SendSysEx(const uint8_t *event, size_t size, uint32_t portNumber) {
    try {
        synth->processSysEx(reinterpret_cast<const char *>(event), size);
    } catch (...) {
        return;
    }
}
