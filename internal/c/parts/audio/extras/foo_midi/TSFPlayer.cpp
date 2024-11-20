//----------------------------------------------------------------------------------------------------------------------
// foo_midi: A foobar2000 component to play MIDI files (https://github.com/stuerp/foo_midi)
// Copyright (c) 2022-2024 Peter Stuer
//
// Modified by a740g for QB64-PE
//----------------------------------------------------------------------------------------------------------------------

#include "TSFPlayer.h"

TSFPlayer::TSFPlayer(InstrumentBankManager *ibm) : MIDIPlayer(), instrumentBankManager(ibm), synth(nullptr) {}

TSFPlayer::~TSFPlayer() {
    Shutdown();
}

bool TSFPlayer::Startup() {
    if (_IsInitialized)
        return true;

    if (!instrumentBankManager || instrumentBankManager->GetType() != InstrumentBankManager::Type::TinySoundFont)
        return false;

    if (instrumentBankManager->GetLocation() == InstrumentBankManager::Location::File)
        synth = tsf_load_filename(instrumentBankManager->GetPath());
    else
        synth = tsf_load_memory(instrumentBankManager->GetData(), instrumentBankManager->GetDataSize());

    if (synth) {
        tsf_channel_set_bank_preset(synth, 9, 128, 0); // initialize preset on special 10th MIDI channel to use percussion sound bank (128) if available
        tsf_set_output(synth, TSF_STEREO_INTERLEAVED, _SampleRate); // set the rendering output mode to stereo interleaved

        _MIDIFlavor = MIDIFlavor::None;
        _FilterEffects = false;
        _IsInitialized = true;

        return true;
    }

    return false;
}

void TSFPlayer::Shutdown() {
    tsf_close(synth);
    synth = nullptr;
    _IsInitialized = false;
}

uint32_t TSFPlayer::GetActiveVoiceCount() const {
    return tsf_active_voice_count(synth);
}

void TSFPlayer::Render(audio_sample *buffer, uint32_t frames) {
    auto data = buffer;

    // We need to render in small blocks because the lower the block size is the more accurate the effects are
    // Sadly, that's how tsf_render_float() works
    while (frames != 0) {
        auto todo = frames > renderFrames ? renderFrames : frames;

        tsf_render_float(synth, data, todo, true);

        data += (todo << 1);
        frames -= todo;
    }
}

void TSFPlayer::SendEvent(uint32_t data) {
    auto channel = uint8_t(data & 0x0F);
    auto command = uint8_t(data & 0xF0);
    auto param1 = uint8_t((data >> 8) & 0x7F);
    auto param2 = uint8_t((data >> 16) & 0x7F);

    switch (command) {
    case StatusCodes::NoteOff:
        tsf_channel_note_off(synth, channel, param1);
        break;

    case StatusCodes::NoteOn:
        tsf_channel_note_on(synth, channel, param1, float(param2) / 127.0f);
        break;

    case StatusCodes::ControlChange:
        tsf_channel_midi_control(synth, channel, param1, param2);
        break;

    case StatusCodes::ProgramChange:
        tsf_channel_set_presetnumber(synth, channel, param1, channel == 9);
        tsf_channel_midi_control(synth, channel, 123, 0); // ALL_NOTES_OFF; https://github.com/schellingb/TinySoundFont/issues/59
        break;

    case StatusCodes::PitchBendChange:
        tsf_channel_set_pitchwheel(synth, channel, uint32_t(param1) | uint32_t(param2 << 7));
        break;
    }
}
