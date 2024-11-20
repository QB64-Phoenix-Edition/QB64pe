
/** $VER: MIDIPlayer.cpp (2023.12.24) **/

#include "MIDIPlayer.h"
#include "../libmidi/framework.h"

/// <summary>
/// Initializes a new instance.
/// </summary>
MIDIPlayer::MIDIPlayer() {
    _SampleRate = 1000;
    _Length = 0;
    _Position = 0;
    _Remainder = 0;
    _LoopBegin = 0;

    _IsInitialized = false;
}

/// <summary>
/// Loads the specified MIDI container.
/// </summary>
bool MIDIPlayer::Load(const midi_container_t &midiContainer, uint32_t subsongIndex, LoopType loopType, uint32_t cleanFlags) {
    _LoopType = loopType;

    assert(_Stream.size() == 0);

    midiContainer.SerializeAsStream(subsongIndex, _Stream, _SysExMap, _StreamLoopBegin, _StreamLoopEnd, cleanFlags);

    if (_Stream.size() == 0)
        return false;

    _StreamPosition = 0;
    _Position = 0;

    _Length = midiContainer.GetDuration(subsongIndex, true);

    if (_LoopType == LoopType::NeverLoopAddDecayTime)
        _Length += DecayTime;
    else if (_LoopType >= LoopType::LoopAndFadeWhenDetected) {
        _LoopBegin = midiContainer.GetLoopBeginTimestamp(subsongIndex, true);

        if (_LoopBegin == std::numeric_limits<decltype(_LoopBegin)>::max())
            _LoopBegin = 0;

        uint32_t LoopEnd = midiContainer.GetLoopEndTimestamp(subsongIndex, true);

        if (LoopEnd == std::numeric_limits<decltype(LoopEnd)>::max())
            LoopEnd = _Length;

        // FIXME: I don't have a clue what this does.
        {
            constexpr size_t NoteOnSize = (size_t)128 * 16;

            std::vector<uint8_t> NoteOn(NoteOnSize, 0);

            {
                size_t i;

                for (i = 0; (i < _Stream.size()) && (i < _StreamLoopEnd); ++i) {
                    uint32_t Message = _Stream[i].Data;

                    uint32_t Event = Message & 0x800000F0;

                    if (Event == StatusCodes::NoteOn || Event == StatusCodes::NoteOff) {
                        const unsigned long Port = (Message >> 24) & 0x7F;
                        const unsigned long Velocity = (Message >> 16) & 0xFF;
                        const unsigned long Note = (Message >> 8) & 0x7F;
                        const unsigned long Channel = Message & 0x0F;

                        const bool IsNoteOn = (Event == StatusCodes::NoteOn) && (Velocity > 0);

                        const unsigned long bit = (unsigned long)(1 << Port);

                        size_t Index = (size_t)Channel * 128 + Note;

                        NoteOn[Index] = (uint8_t)((NoteOn[Index] & ~bit) | (bit * IsNoteOn));
                    }
                }

                _Stream.resize(i);

                _Length = std::max(LoopEnd - 1, _Stream[i - 1].Time);
            }

            for (size_t i = 0; i < NoteOnSize; ++i) {
                if (NoteOn[i] != 0x00) {
                    for (size_t j = 0; j < 8; ++j) {
                        if (NoteOn[i] & (1 << j)) {
                            _Stream.push_back(midi_item_t(_Length, (uint32_t)((j << 24) + (i >> 7) + ((i & 0x7F) << 8) + 0x90)));
                        }
                    }
                }
            }

            _Length = LoopEnd;
        }
    }

    if (_SampleRate != 1000) {
        uint32_t NewSampleRate = _SampleRate;

        _SampleRate = 1000;

        SetSampleRate(NewSampleRate);
    }

    return true;
}

/// <summary>
/// Renders the specified number of samples to an audio sample buffer.
/// </summary>
/// <remarks>All calculations are in samples. MIDIStreamEvent::Timestamp gets converted from ms to samples before playing starts.</remarks>
uint32_t MIDIPlayer::Play(audio_sample *sampleData, uint32_t sampleCount) noexcept {
    assert(_Stream.size());

    if (!Startup())
        return 0;

    const uint32_t BlockSize = GetSampleBlockSize();

    uint32_t SampleIndex = 0;
    uint32_t BlockOffset = 0;

    while ((SampleIndex < sampleCount) && (_Remainder > 0)) {
        uint32_t Remainder = _Remainder;

        {
            if (Remainder > sampleCount - SampleIndex)
                Remainder = sampleCount - SampleIndex;

            if ((BlockSize != 0) && (Remainder > BlockSize))
                Remainder = BlockSize;
        }

        if (Remainder < BlockSize) {
            _Remainder = 0;
            BlockOffset = Remainder;
            break;
        }

        {
            Render(sampleData + (SampleIndex * 2), Remainder);

            SampleIndex += Remainder;
            _Position += Remainder;
        }

        _Remainder -= Remainder;
    }

    while (SampleIndex < sampleCount) {
        uint32_t Remainder = _Length - _Position;

        if (Remainder > sampleCount - SampleIndex)
            Remainder = sampleCount - SampleIndex;

        const uint32_t NewPosition = _Position + Remainder;

        {
            size_t NewStreamPosition = _StreamPosition;

            while ((NewStreamPosition < _Stream.size()) && (_Stream[NewStreamPosition].Time < NewPosition))
                NewStreamPosition++;

            if (NewStreamPosition > _StreamPosition) {
                for (; _StreamPosition < NewStreamPosition; ++_StreamPosition) {
                    const midi_item_t &mse = _Stream[_StreamPosition];

                    int64_t ToDo = (int64_t)mse.Time - (int64_t)_Position - (int64_t)BlockOffset;

                    if (ToDo > 0) {
                        if (ToDo > (int64_t)(sampleCount - SampleIndex)) {
                            _Remainder = (uint32_t)(ToDo - (int64_t)(sampleCount - SampleIndex));
                            ToDo = (int64_t)(sampleCount - SampleIndex);
                        }

                        if ((ToDo > 0) && (BlockSize == 0)) {
                            Render(sampleData + (SampleIndex * 2), (uint32_t)ToDo);

                            SampleIndex += (uint32_t)ToDo;
                            _Position += (uint32_t)ToDo;
                        }

                        if (_Remainder > 0) {
                            _Remainder += BlockOffset;

                            return SampleIndex;
                        }
                    }

                    if (BlockSize == 0)
                        SendEventFiltered(mse.Data);
                    else {
                        BlockOffset += (uint32_t)ToDo;

                        while (BlockOffset >= BlockSize) {
                            Render(sampleData + (SampleIndex * 2), BlockSize);

                            SampleIndex += BlockSize;
                            BlockOffset -= BlockSize;
                            _Position += BlockSize;
                        }

                        SendEventFiltered(mse.Data, BlockOffset);
                    }
                }
            }
        }

        if (SampleIndex < sampleCount) {
            Remainder = ((_StreamPosition < _Stream.size()) ? _Stream[_StreamPosition].Time : _Length) - _Position;

            if (BlockSize != 0)
                BlockOffset = Remainder;

            {
                if (Remainder > sampleCount - SampleIndex)
                    Remainder = sampleCount - SampleIndex;

                if ((BlockSize != 0) && (Remainder > BlockSize))
                    Remainder = BlockSize;
            }

            if (Remainder >= BlockSize) {
                {
                    Render(sampleData + (SampleIndex * 2), Remainder);

                    SampleIndex += Remainder;
                    _Position += Remainder;
                }

                if (BlockSize != 0)
                    BlockOffset -= Remainder;
            }
        }

        if (BlockSize == 0)
            _Position = NewPosition;

        // Have we reached the end of the song?
        if (NewPosition >= _Length) {
            // Process any remaining events.
            for (; _StreamPosition < _Stream.size(); _StreamPosition++) {
                if (BlockSize == 0)
                    SendEventFiltered(_Stream[_StreamPosition].Data);
                else
                    SendEventFiltered(_Stream[_StreamPosition].Data, BlockOffset);
            }

            if ((_LoopType == LoopType::LoopAndFadeWhenDetected) || (_LoopType == LoopType::PlayIndefinitelyWhenDetected)) {
                if (_StreamLoopBegin != ~0) {
                    _StreamPosition = _StreamLoopBegin;
                    _Position = _LoopBegin;
                } else
                    break;
            } else if ((_LoopType == LoopType::LoopAndFadeAlways) || (_LoopType == LoopType::PlayIndefinitely)) {
                _StreamPosition = 0;
                _Position = 0;
            } else
                break;
        }
    }

    _Remainder = BlockOffset;

    return SampleIndex;
}

/// <summary>
/// Seeks to the specified time (in samples)
/// </summary>
void MIDIPlayer::Seek(uint32_t timeInSamples) {
    if (timeInSamples >= _Length) {
        if (_LoopType >= LoopType::LoopAndFadeWhenDetected) {
            while (timeInSamples >= _Length)
                timeInSamples -= _Length - _LoopBegin;
        } else {
            timeInSamples = _Length;
        }
    }

    if (_Position > timeInSamples) {
        _StreamPosition = 0;

        if (!Reset())
            Shutdown();
    }

    if (!Startup())
        return;

    _Position = timeInSamples;

    size_t OldStreamPosition = _StreamPosition;

    {
        // Find the position in the MIDI stream that corresponds with the seek time.
        for (; (_StreamPosition < _Stream.size()) && (_Stream[_StreamPosition].Time < _Position); _StreamPosition++)
            ;

        if (_StreamPosition == _Stream.size())
            _Remainder = _Length - _Position;
        else
            _Remainder = _Stream[_StreamPosition].Time - _Position;
    }

    if (_StreamPosition <= OldStreamPosition)
        return;

    std::vector<midi_item_t> FillerEvents(_StreamPosition - OldStreamPosition);

    FillerEvents.assign(&_Stream[OldStreamPosition], &_Stream[_StreamPosition]);

    for (size_t i = 0; i < FillerEvents.size(); ++i) {
        midi_item_t &mse1 = FillerEvents[i];

        if ((mse1.Data & 0x800000F0) == 0x90 && (mse1.Data & 0x00FF0000)) // note on
        {
            if ((mse1.Data & 0x0F) == 9) // hax
            {
                mse1.Data = 0;
                continue;
            }

            const uint32_t m1 = (mse1.Data & 0x7F00FF0F) | 0x80; // note off
            const uint32_t m2 = (mse1.Data & 0x7F00FFFF);        // also note off

            for (size_t j = i + 1; j < FillerEvents.size(); ++j) {
                midi_item_t &mse2 = FillerEvents[j];

                if ((mse2.Data & 0xFF00FFFF) == m1 || mse2.Data == m2) {
                    // kill 'em
                    mse1.Data = 0;
                    mse2.Data = 0;
                    break;
                }
            }
        }
    }

    const uint32_t BlockSize = GetSampleBlockSize();

    if (BlockSize != 0) {
        audio_sample *Temp = new audio_sample[BlockSize * 2];

        if (Temp != nullptr) {
            Render(Temp, BlockSize); // Flush events

            uint32_t JunkSize = 0;

            uint32_t LastTimestamp = 0;
            bool IsTimestampSet = false;

            for (const midi_item_t &Event : FillerEvents) {
                if (Event.Data != 0) {
                    SendEventFiltered(Event.Data, JunkSize);

                    if (IsTimestampSet && (Event.Time != LastTimestamp))
                        JunkSize += 16;

                    LastTimestamp = Event.Time;
                    IsTimestampSet = true;

                    if (JunkSize >= BlockSize) {
                        Render(Temp, BlockSize); // Flush events
                        JunkSize -= BlockSize;
                    }
                }
            }

            Render(Temp, BlockSize); // Flush events

            delete[] Temp;
        }
    } else {
        audio_sample *Temp = new audio_sample[16 * 2];

        if (Temp != nullptr) {
            Render(Temp, 16); // Flush events

            uint32_t LastTimestamp = 0;
            bool IsTimestampSet = false;

            for (const midi_item_t &Event : FillerEvents) {
                if (Event.Data != 0) {
                    if (IsTimestampSet && (Event.Time != LastTimestamp))
                        Render(Temp, 16); // Flush events

                    LastTimestamp = Event.Time;
                    IsTimestampSet = true;

                    SendEventFiltered(Event.Data);
                }
            }

            Render(Temp, 16); // Flush events

            delete[] Temp;
        }
    }
}

/// <summary>
/// Converts the timestamps of the MIDI stream (in ms) to timestamps in samples. Note: that's why the default sample rate is 1000: to force a recalculation
/// before starting to play.
/// </summary>
void MIDIPlayer::SetSampleRate(uint32_t sampleRate) {
    if (sampleRate == _SampleRate)
        return;

    for (midi_item_t &it : _Stream)
        it.Time = (uint32_t)MulDiv((int)it.Time, (int)sampleRate, (int)_SampleRate);

    if (_Length != 0)
        _Length = (uint32_t)MulDiv((int)_Length, (int)sampleRate, (int)_SampleRate);

    if (_LoopBegin != 0)
        _LoopBegin = (uint32_t)MulDiv((int)_LoopBegin, (int)sampleRate, (int)_SampleRate);

    if (_Position != 0)
        _Position = (uint32_t)MulDiv((int)_Position, (int)sampleRate, (int)_SampleRate);

    _SampleRate = sampleRate;

    Shutdown();
}

/// <summary>
/// Configures the MIDI player.
/// </summary>
void MIDIPlayer::Configure(MIDIFlavor midiFlavor, bool filterEffects) {
    _MIDIFlavor = midiFlavor;
    _FilterEffects = filterEffects;

    if (_IsInitialized) {
        SendSysExReset(0, 0);
        SendSysExReset(1, 0);
        SendSysExReset(2, 0);
    }
}

/// <summary>
///
/// </summary>
void MIDIPlayer::SendEventFiltered(uint32_t data) {
    if (!(data & 0x80000000u)) {
        if (_FilterEffects) {
            const uint32_t Data = data & 0x00007FF0u;

            // Filter Control Change "Effects 1 (External Effects) Depth" (0x5B) and "Effects 3 (Chorus) Depth" (0x5D).
            if (Data == 0x5BB0 || Data == 0x5DB0)
                return;
        }

        SendEvent(data);
    } else {
        const uint32_t Index = data & 0x00FFFFFFu;

        const uint8_t *Data;
        size_t Size;
        uint8_t Port;

        if (_SysExMap.GetItem(Index, Data, Size, Port))
            SendSysExFiltered(Data, Size, Port);
    }
}

/// <summary>
///
/// </summary>
void MIDIPlayer::SendEventFiltered(uint32_t data, uint32_t time) {
    if (!(data & 0x80000000u)) {
        if (_FilterEffects) {
            const uint32_t Data = data & 0x00007FF0u;

            // Filter Control Change "Effects 1 (External Effects) Depth" (0x5B) and "Effects 3 (Chorus) Depth" (0x5D)
            if (Data == 0x5BB0 || Data == 0x5DB0)
                return;
        }

        SendEvent(data, time);
    } else {
        const uint32_t Index = data & 0x00FFFFFFu;

        const uint8_t *Data;
        size_t Size;
        uint8_t Port;

        if (_SysExMap.GetItem(Index, Data, Size, Port))
            SendSysExFiltered(Data, Size, Port, time);
    }
}

static const uint8_t SysExResetGM[] = {0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7};
static const uint8_t SysExResetGM2[] = {0xF0, 0x7E, 0x7F, 0x09, 0x03, 0xF7};
static const uint8_t SysExResetGS[] = {0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7};
static const uint8_t SysExResetXG[] = {0xF0, 0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00, 0xF7};

static const uint8_t SysExGSToneMapNumber[] = {0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x41, 0x00, 0x03, 0x00, 0xF7};

static bool IsSysExReset(const uint8_t *data);
static bool IsSysExEqual(const uint8_t *a, const uint8_t *b);

/// <summary>
/// Sends a SysEx.
/// </summary>
void MIDIPlayer::SendSysExFiltered(const uint8_t *data, size_t size, uint8_t portNumber) {
    SendSysEx(data, size, portNumber);

    if (IsSysExReset(data) && (_MIDIFlavor != MIDIFlavor::None))
        SendSysExReset(portNumber, 0);
}

/// <summary>
/// Sends a SysEx with a timestamp.
/// </summary>
void MIDIPlayer::SendSysExFiltered(const uint8_t *data, size_t size, uint8_t portNumber, uint32_t time) {
    SendSysEx(data, size, portNumber, time);

    if (IsSysExReset(data) && (_MIDIFlavor != MIDIFlavor::None))
        SendSysExReset(portNumber, time);
}

/// <summary>
/// Sends a SysEx reset message.
/// </summary>
void MIDIPlayer::SendSysExReset(uint8_t portNumber, uint32_t time) {
    if (!_IsInitialized)
        return;

    if (time != 0) {
        SendSysEx(SysExResetXG, sizeof(SysExResetXG), portNumber, time);
        SendSysEx(SysExResetGM2, sizeof(SysExResetGM2), portNumber, time);
        SendSysEx(SysExResetGM, sizeof(SysExResetGM), portNumber, time);
    } else {
        SendSysEx(SysExResetXG, sizeof(SysExResetXG), portNumber);
        SendSysEx(SysExResetGM2, sizeof(SysExResetGM2), portNumber);
        SendSysEx(SysExResetGM, sizeof(SysExResetGM), portNumber);
    }

    switch (_MIDIFlavor) {
    case MIDIFlavor::GM:
        if (time != 0)
            SendSysEx(SysExResetGM, sizeof(SysExResetGM), portNumber, time);
        else
            SendSysEx(SysExResetGM, sizeof(SysExResetGM), portNumber);
        break;

    case MIDIFlavor::GM2:
        if (time != 0)
            SendSysEx(SysExResetGM2, sizeof(SysExResetGM2), portNumber, time);
        else
            SendSysEx(SysExResetGM2, sizeof(SysExResetGM2), portNumber);
        break;

    case MIDIFlavor::SC55:
    case MIDIFlavor::SC88:
    case MIDIFlavor::SC88Pro:
    case MIDIFlavor::SC8850:
    case MIDIFlavor::None:
        if (time != 0)
            SendSysEx(SysExResetGS, sizeof(SysExResetGS), portNumber, time);
        else
            SendSysEx(SysExResetGS, sizeof(SysExResetGS), portNumber);

        SendSysExSetToneMapNumber(portNumber, time);
        break;

    case MIDIFlavor::XG:
        if (time != 0)
            SendSysEx(SysExResetXG, sizeof(SysExResetXG), portNumber, time);
        else
            SendSysEx(SysExResetXG, sizeof(SysExResetXG), portNumber);
        break;
    }

    {
        for (uint8_t i = 0; i < 16; ++i) {
            if (time != 0) {
                SendEvent((uint32_t)((0x78B0 + i) + (portNumber << 24)), time); // CC 120 Channel Mute / Sound Off
                SendEvent((uint32_t)((0x79B0 + i) + (portNumber << 24)), time); // CC 121 Reset All Controllers

                if (_MIDIFlavor != MIDIFlavor::XG || i != 9) {
                    SendEvent((uint32_t)((0x20B0 + i) + (portNumber << 24)), time); // CC 32 Bank select LSB
                    SendEvent((uint32_t)((0x00B0 + i) + (portNumber << 24)), time); // CC  0 Bank select MSB
                    SendEvent((uint32_t)((0x00C0 + i) + (portNumber << 24)), time); // Program Change 0
                }
            } else {
                SendEvent((uint32_t)((0x78B0 + i) + (portNumber << 24))); // CC 120 Channel Mute / Sound Off
                SendEvent((uint32_t)((0x79B0 + i) + (portNumber << 24))); // CC 121 Reset All Controllers

                if (_MIDIFlavor != MIDIFlavor::XG || i != 9) {
                    SendEvent((uint32_t)((0x20B0 + i) + (portNumber << 24))); // CC 32 Bank select LSB
                    SendEvent((uint32_t)((0x00B0 + i) + (portNumber << 24))); // CC  0 Bank select MSB
                    SendEvent((uint32_t)((0x00C0 + i) + (portNumber << 24))); // Program Change 0
                }
            }
        }
    }

    // Configure channel 10 as drum kit in XG mode.
    if (_MIDIFlavor == MIDIFlavor::XG) {
        if (time != 0) {
            SendEvent((uint32_t)(0x0020B9 + (portNumber << 24)), time); // CC 32 Bank select LSB
            SendEvent((uint32_t)(0x7F00B9 + (portNumber << 24)), time); // CC  0 Bank select MSB. Selects Drum Kit in XG mode.
            SendEvent((uint32_t)(0x0000C9 + (portNumber << 24)), time); // Program Change 0
        } else {
            SendEvent((uint32_t)(0x0020B9 + (portNumber << 24))); // CC 32 Bank select LSB
            SendEvent((uint32_t)(0x7F00B9 + (portNumber << 24))); // CC  0 Bank select MSB. Selects Drum Kit in XG mode.
            SendEvent((uint32_t)(0x0000C9 + (portNumber << 24))); // Program Change 0
        }
    }

    if (_FilterEffects) {
        if (time != 0) {
            for (uint8_t i = 0; i < 16; ++i) {
                SendEvent((uint32_t)(0x5BB0 + i + (portNumber << 24)), time); // CC 91 Effect 1 (Reverb) Set Level to 0
                SendEvent((uint32_t)(0x5DB0 + i + (portNumber << 24)), time); // CC 93 Effect 3 (Chorus) Set Level to 0
            }
        } else {
            for (uint8_t i = 0; i < 16; ++i) {
                SendEvent((uint32_t)(0x5BB0 + i + (portNumber << 24))); // CC 91 Effect 1 (Reverb) Set Level to 0
                SendEvent((uint32_t)(0x5DB0 + i + (portNumber << 24))); // CC 93 Effect 3 (Chorus) Set Level to 0
            }
        }
    }
}

/// <summary>
/// Sends a GS SET TONE MAP-0 NUMBER message.
/// </summary>
void MIDIPlayer::SendSysExSetToneMapNumber(uint8_t portNumber, uint32_t time) {
    uint8_t Data[11] = {0};

    ::memcpy(Data, SysExGSToneMapNumber, sizeof(Data));

    Data[7] = 1; // Tone Map-0 Number

    switch (_MIDIFlavor) {
    case MIDIFlavor::SC55:
        Data[8] = 1;
        break;

    case MIDIFlavor::SC88:
        Data[8] = 2;
        break;

    case MIDIFlavor::SC88Pro:
        Data[8] = 3;
        break;

    case MIDIFlavor::SC8850:
    case MIDIFlavor::None:
        Data[8] = 4;
        break;

    case MIDIFlavor::GM:
    case MIDIFlavor::GM2:
    case MIDIFlavor::XG:
    default:
        break; // Use SC88Pro Map (3)
    }

    for (uint8_t i = 0x41; i <= 0x49; ++i) {
        Data[6] = i;
        SendSysExGS(Data, sizeof(Data), portNumber, time);
    }

    {
        Data[6] = 0x40;
        SendSysExGS(Data, sizeof(Data), portNumber, time);
    }

    for (uint8_t i = 0x4A; i <= 0x4F; ++i) {
        Data[6] = i;
        SendSysExGS(Data, sizeof(Data), portNumber, time);
    }
}

/// <summary>
/// Sends a Roland GS message after re-calculating the checksum.
/// </summary>
void MIDIPlayer::SendSysExGS(uint8_t *data, size_t size, uint8_t portNumber, uint32_t time) {
    uint8_t Checksum = 0;
    size_t i;

    for (i = 5; (i + 1 < size) && (data[i + 1] != StatusCodes::SysExEnd); ++i)
        Checksum += data[i];

    data[i] = (uint8_t)((128 - Checksum) & 127);

    if (time > 0)
        SendSysEx(data, size, portNumber, time);
    else
        SendSysEx(data, size, portNumber);
}

static bool IsSysExReset(const uint8_t *data) {
    return IsSysExEqual(data, SysExResetGM) || IsSysExEqual(data, SysExResetGM2) || IsSysExEqual(data, SysExResetGS) || IsSysExEqual(data, SysExResetXG);
}

static bool IsSysExEqual(const uint8_t *a, const uint8_t *b) {
    while ((*a != StatusCodes::SysExEnd) && (*b != StatusCodes::SysExEnd) && (*a == *b)) {
        a++;
        b++;
    }

    return (*a == *b);
}

#ifdef _WIN32
static uint16_t GetWord(const uint8_t *data) noexcept {
    return (uint16_t)(data[0] | (((uint16_t)data[1]) << 8));
}

static uint32_t GetDWord(const uint8_t *data) noexcept {
    return data[0] | (((uint32_t)data[1]) << 8) | (((uint32_t)data[2]) << 16) | (((uint32_t)data[3]) << 24);
}

/// <summary>
/// Determines the processor architecture of a Windows binary file.
/// </summary>
uint32_t MIDIPlayer::GetProcessorArchitecture(const std::string &filePath) const {
    constexpr size_t MZHeaderSize = 0x40;
    constexpr size_t PEHeaderSize = (size_t)4 + 20 + 224;

    uint8_t PEHeader[PEHeaderSize];

    FILE *file = fopen(filePath.c_str(), "rb");

    if (!file)
        return 0;

    // Read the MZ header
    if (fread(PEHeader, 1, MZHeaderSize, file) != MZHeaderSize) {
        fclose(file);
        return 0;
    }

    // Check if it is a valid MZ file
    if (GetWord(PEHeader) != 0x5A4D) {
        fclose(file);
        return 0;
    }

    // Get the offset of the PE header
    uint32_t OffsetPEHeader = GetDWord(PEHeader + 0x3C);

    // Move to the PE header
    if (fseek(file, OffsetPEHeader, SEEK_SET) != 0) {
        fclose(file);
        return 0;
    }

    // Read the PE header
    if (fread(PEHeader, 1, PEHeaderSize, file) != PEHeaderSize) {
        fclose(file);
        return 0;
    }

    // Check if it is a valid PE file
    if (GetDWord(PEHeader) != 0x00004550) {
        fclose(file);
        return 0;
    }

    fclose(file);

    // Get the machine type and determine architecture
    switch (GetWord(PEHeader + 4)) {
    case 0x014C:
        return 32;

    case 0x8664:
        return 64;
    }

    return 0;
}
#endif
