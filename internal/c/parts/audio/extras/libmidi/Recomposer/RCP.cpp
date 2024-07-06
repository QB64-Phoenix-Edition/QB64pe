
/** $VER: RCP.cpp (2024.05.22) P. Stuer - Based on Valley Bell's rpc2mid (https://github.com/ValleyBell/MidiConverters). **/

#include "RCP.h"
#include "RunningNotes.h"

extern running_notes_t RunningNotes;

#define MCMD_INI_EXCLUDE 0x00  // exclude initial command
#define MCMD_INI_INCLUDE 0x01  // include initial command
#define MCMD_RET_CMDCOUNT 0x00 // return number of commands
#define MCMD_RET_DATASIZE 0x02 // return number of data bytes

static uint8_t DetermineShift(uint32_t value);
static uint16_t ConvertRCPSysExToMIDISysEx(const uint8_t *srcData, uint16_t srcSize, uint8_t *dstData, uint8_t param1, uint8_t param2, uint8_t channel);

/// <summary>
///
/// </summary>
void rcp_file_t::ReadTrack(const uint8_t *data, uint32_t size, uint32_t offset, rcp_track_t *rcpTrack) const {
    if (offset >= size)
        throw std::runtime_error("Insufficient data to read track.");

    uint32_t Offset = offset;

    const uint32_t TrackHead = Offset;

    uint32_t TrackSize = 0;

    if (_Version == 2) {
        TrackSize = ReadLE16(data + Offset);
        Offset += 2;
    } else if (_Version == 3) {
        TrackSize = ReadLE32(data + Offset);
        Offset += 4;
    }

    const uint32_t TailOffset = std::min(TrackHead + TrackSize, size);

    if (Offset + 0x2A > size)
        throw std::runtime_error("Insufficient data to read track header.");

    Offset += 0x2A; // Skip the track header.

    rcpTrack->Offs = TrackHead;
    rcpTrack->Size = TrackSize;
    rcpTrack->Duration = 0;

    rcpTrack->LoopStartOffs = 0;
    rcpTrack->LoopStartTick = 0;

    std::vector<uint32_t> MeasureOffsets;

    MeasureOffsets.reserve(256);

#ifdef _RCP_VERBOSE
    ::printf("%04X: Track begin\n", Offset);
#endif

    MeasureOffsets.push_back(Offset);

    bool EndOfTrack = false;

    uint32_t ParentOffs = 0;

    struct loop_t {
        uint32_t ParentOffs;
        uint32_t StartOffs;
        uint32_t StartTick;
        uint16_t Counter;
    };

    std::vector<loop_t> Loops;

    Loops.reserve(8);

    uint8_t LoopCount = 0;

    uint32_t LoopParentOffs[8] = {};
    uint32_t LoopStartOffs[8] = {};
    uint32_t LoopStartTick[8] = {};
    uint16_t LoopCounter[8] = {};

    while ((Offset < TailOffset) && !EndOfTrack) {
        uint8_t CmdType = 0;
        uint16_t CmdP0 = 0;
        uint8_t CmdP1 = 0;
        uint8_t CmdP2 = 0;
        uint16_t CmdDuration = 0;

        if (_Version == 2) {
            CmdType = data[Offset + 0x00];
            CmdP0 = data[Offset + 0x01];
            CmdP1 = data[Offset + 0x02];
            CmdP2 = data[Offset + 0x03];
            CmdDuration = CmdP1;

            Offset += 4;
        } else if (_Version == 3) {
            CmdType = data[Offset + 0x00];
            CmdP2 = data[Offset + 0x01];
            CmdP0 = ReadLE16(&data[Offset + 0x02]);
            CmdP1 = data[Offset + 0x04];
            CmdDuration = ReadLE16(&data[Offset + 0x04]);

            Offset += 6;
        }

        switch (CmdType) {
        case 0xF8: // Loop End
        {
            if (LoopCount > 0) {
                LoopCount--;

                LoopCounter[LoopCount]++;

                if (CmdP0 == 0) {
                    rcpTrack->LoopStartOffs = LoopStartOffs[LoopCount];
                    rcpTrack->LoopStartTick = LoopStartTick[LoopCount];

                    EndOfTrack = 1;
                } else {
                    if (LoopCounter[LoopCount] < CmdP0) {
                        ParentOffs = LoopParentOffs[LoopCount];
                        Offset = LoopStartOffs[LoopCount];

                        LoopCount++;
                    }
                }
            }

            CmdP0 = 0;
            break;
        }

        case 0xF9: // Loop Begin
        {
            if (LoopCount < 8) {
                LoopParentOffs[LoopCount] = ParentOffs;
                LoopStartOffs[LoopCount] = Offset;
                LoopStartTick[LoopCount] = rcpTrack->Duration;
                LoopCounter[LoopCount] = 0;

                LoopCount++;
            }

            Loops.push_back({ParentOffs, Offset, rcpTrack->Duration, 0});

            CmdP0 = 0;
            break;
        }

        case 0xFC: // Repeat previous bar
        {
            if (ParentOffs) {
                Offset = ParentOffs;
                ParentOffs = 0x00;
            } else {
                if (_Version == 2)
                    Offset -= 0x04;
                else if (_Version == 3)
                    Offset -= 0x06;
                do {
                    uint32_t OldOffset = Offset;

                    uint16_t MeasureId = 0;
                    uint32_t RepeatOffset = 0;

                    if (_Version == 2) {
                        CmdP0 = data[Offset + 0x01];
                        CmdP1 = data[Offset + 0x02];
                        CmdP2 = data[Offset + 0x03];

                        MeasureId = (uint16_t)(CmdP0 | ((CmdP1 & 0x03) << 8));
                        RepeatOffset = (uint32_t)((CmdP1 & ~0x03) | (CmdP2 << 8));

                        Offset += 0x04;
                    } else if (_Version == 3) {
                        MeasureId = ReadLE16(&data[Offset + 0x02]);
                        RepeatOffset = (uint32_t)(0x002E + (ReadLE16(&data[Offset + 0x04]) - 0x0030) * 0x06);

                        Offset += 0x06;
                    }
                    if (MeasureId >= MeasureOffsets.size())
                        break;

                    if (TrackHead + RepeatOffset == OldOffset)
                        break; // prevent recursion

                    if (!ParentOffs) // necessary for following FC command chain
                        ParentOffs = Offset;

                    Offset = TrackHead + RepeatOffset;
                } while (data[Offset] == 0xFC);
            }

            CmdP0 = 0;
            break;
        }

        case 0xFD: // Measure end
        {
            if (MeasureOffsets.size() >= 0x8000) {
#ifdef _RCP_VERBOSE
                ::printf("Warning: too many measures in track.\n");
#endif

                EndOfTrack = 1;
                break;
            }
            if (ParentOffs) {
                Offset = ParentOffs;
                ParentOffs = 0x00;
            }

            MeasureOffsets.push_back(Offset);

            CmdP0 = 0;

            if (_Options._WolfteamLoopMode && MeasureOffsets.size() == 2) {
                LoopCount = 0;

                LoopParentOffs[LoopCount] = ParentOffs;
                LoopStartOffs[LoopCount] = Offset;
                LoopStartTick[LoopCount] = rcpTrack->Duration;
                LoopCounter[LoopCount] = 0;

                LoopCount++;

                Loops.clear();

                Loops.push_back({ParentOffs, Offset, rcpTrack->Duration, 0});
            }
            break;
        }

        case 0xFE: // Track end
        {
            EndOfTrack = true;
            CmdP0 = 0;

            if (_Options._WolfteamLoopMode && (LoopCount > 0)) {
                LoopCount = 0;

                rcpTrack->LoopStartOffs = LoopStartOffs[LoopCount];
                rcpTrack->LoopStartTick = LoopStartTick[LoopCount];

                Loops.clear();
            }
            break;
        }

        default: {
            if (CmdType >= 0xF0)
                CmdP0 = 0;
        }
        }

        rcpTrack->Duration += CmdP0;
    }

#ifdef _RCP_VERBOSE
    ::printf("%04X: Track End: %d measures, %d ticks.\n\n", Offset, (int)MeasureOffsets.size(), rcpTrack->Duration);
#endif
}

/// <summary>
/// Converts an RCP track to a MIDI track.
/// </summary>
void rcp_file_t::ConvertTrack(const uint8_t *data, uint32_t size, uint32_t *offset, rcp_track_t *track, midi_stream_t &midiStream) const {
    uint32_t Offset = *offset;

    if (Offset >= size)
        throw std::runtime_error("Invalid start of track position.");

    const uint32_t TrackHead = Offset;

    uint32_t TrackSize = 0;

    if (_Version == 2) {
        TrackSize = ReadLE16(&data[Offset]);

        // Bits 0/1 are used as 16/17, allowing for up to 256 KB per track. This is used by some ItoR.x conversions.
        TrackSize = (TrackSize & ~0x03) | ((TrackSize & 0x03) << 16);
        Offset += 2;
    } else if (_Version == 3) {
        TrackSize = ReadLE32(&data[Offset]);
        Offset += 4;
    }

    uint32_t TrackTail = std::min(TrackHead + TrackSize, size);

    if (Offset + 0x2A > size)
        throw std::runtime_error("Insufficient data to read track header.");

    uint8_t TrackId = data[Offset + 0x00];    // Track ID (1-based)
    uint8_t RhythmMode = data[Offset + 0x01]; // Rhythm mode (0x00 - off, 0x80 - on, others undefined / fall back to off)
    uint8_t SrcChannel = data[Offset + 0x02]; // MIDI channel (0xFF = null device (Don't play), 0x00..0x0F = port A ch 0..15, 0x10..0x1F = port B ch 0..15)
    uint8_t DstChannel = 0x00;

    if (SrcChannel & 0x80) {
        // When the KeepMutedChannels option is off, prevent events from being written to the MIDI file by setting DstChannel to 0xFF.
        DstChannel = _Options._KeepDummyChannels ? (uint8_t)0x00 : (uint8_t)0xFF;
        SrcChannel = 0x00;
    } else {
        DstChannel = (uint8_t)(SrcChannel >> 4);
        SrcChannel &= 0x0F;
    }

    int8_t Transposition = (int8_t)data[Offset + 0x03]; // Key Offset
    int32_t StartTick = (int32_t)(int8_t)data[Offset + 0x04];
    uint8_t TrackMute = data[Offset + 0x05];

    rcp_string_t TrackName;

    TrackName.Assign(&data[Offset + 0x06], 0x24);

    Offset += 0x2A; // Skip the track header.

#ifdef _RCP_VERBOSE
    ::printf("Track %02X, \"%.*s\"\n", TrackId, TrackName.Len, TrackName.Data);
    ::printf("    Rhythm Mode: %02X, Src Channel: %02X, Dst Channel: %02X, Transposition: %4d, Start Tick: %6d, Mute Mode: %02X \n", RhythmMode, SrcChannel,
             DstChannel, Transposition, StartTick, TrackMute);
#endif

    // Write a conductor track with the initial setup.
    {
        uint32_t OldDuration = midiStream.GetDuration();

        midiStream.SetDuration(0); // Make sure all events on the conductor track have timestamp 0.

        if (TrackName.Len > 0)
            midiStream.WriteMetaEvent(MetaDataTypes::TrackName, TrackName.Data, TrackName.Len);

        // 0x00 == off, 0x01 == on. Others are undefined. Fall back to 'off'?
        if (TrackMute && !_Options._KeepDummyChannels) {
            // Ignore muted tracks.
            *offset = TrackHead + TrackSize;

            return;
        }

        if (DstChannel != 0xFF) {
            midiStream.WriteMetaEvent(MetaDataTypes::MIDIPort, &DstChannel, 1);
            midiStream.WriteMetaEvent(MetaDataTypes::ChannelPrefix, &SrcChannel, 1);
        }

        // For RhythmMode, values 0 (melody channel) and 0x80 (rhythm channel) are common.
        // Some songs use different values, but the actual meaning of the value is unknown.
#ifdef _RCP_VERBOSE
        if ((RhythmMode != 0) && (RhythmMode != 0x80))
            ::printf("Warning: Track %2u: Unknown Rhythm Mode 0x%02X.\n", TrackId, RhythmMode);
#endif

        // Transposition in semitones, 7-bit signed: 0 = no transposition, 0x0C = +1 octave, 0x74 = -1 octave, added together with global transposition,
        // 0x80..0xFF = rhythm track (no per-track transposition, ignores global transposition as well).
        if (Transposition & 0x80) {
            Transposition = 0; // Ignore the transposition for rhythm channels.
        } else {
            Transposition = (Transposition & 0x40) ? (-0x80 + Transposition) : Transposition; // 7-bit -> 8-bit sign extension
            Transposition += _GlobalTransposition;
        }

        midiStream.SetDuration(OldDuration);

        RunningNotes.Reset();
    }

    midiStream.SetChannel(SrcChannel);

    // Add "StartTick" offset to the initial timestamp.
    {
        uint32_t Timestamp = midiStream.GetDuration();

        if ((StartTick >= 0) || (-StartTick <= (int32_t)Timestamp)) {
            Timestamp += StartTick;
            StartTick = 0;
        } else {
            StartTick += Timestamp;
            Timestamp = 0;
        }

        midiStream.SetDuration(Timestamp);
    }

    {
        uint8_t Temp[256];

        uint8_t GSParameters[6] = {}; // 0 device ID, 1 model ID, 2 address high, 3 address low
        uint8_t XGParameters[6] = {}; // 0 device ID, 1 model ID, 2 address high, 3 address low

        uint32_t ParentOffs = 0;
        uint16_t RepeatingBarID = 0xFFFF;

        std::vector<uint32_t> BarOffsets;
        uint16_t BarCount = 0;

        BarOffsets.reserve(256);

        BarOffsets.push_back(Offset);

        buffer_t Text;
        bool EndOfTrack = false;

        uint8_t LoopCount = 0;

        uint32_t LoopParentOffs[8] = {};
        uint32_t LoopStartOffs[8] = {};
        uint16_t LoopCounter[8] = {};

        while ((Offset < TrackTail) && !EndOfTrack) {
            uint32_t CmdOffset = Offset; // Offset of the start of the command

            uint8_t CmdType = 0;
            uint16_t CmdP0 = 0;
            uint8_t CmdP1 = 0;
            uint8_t CmdP2 = 0;
            uint16_t CmdDuration = 0;

            if (_Version == 2) {
                CmdType = data[Offset + 0x00];
                CmdP0 = data[Offset + 0x01];
                CmdP1 = data[Offset + 0x02];
                CmdP2 = data[Offset + 0x03];
                CmdDuration = CmdP1;

                Offset += 4;
            } else if (_Version == 3) {
                CmdType = data[Offset + 0x00];
                CmdP2 = data[Offset + 0x01];
                CmdP0 = ReadLE16(&data[Offset + 0x02]);
                CmdP1 = data[Offset + 0x04];
                CmdDuration = ReadLE16(&data[Offset + 0x04]);

                Offset += 6;
            }

            if (CmdType < 0x80) {
                // It's a note.
                uint8_t Note = (uint8_t)((CmdType + Transposition) & 0x7F);

                {
                    {
                        uint32_t Duration = midiStream.GetDuration();

                        RunningNotes.Check(midiStream, Duration);

                        midiStream.SetDuration(Duration);
                    }

                    for (uint16_t i = 0; i < RunningNotes._Count; ++i) {
                        if (RunningNotes._Notes[i].Note == Note) {
                            // The note is already playing. Remember its new duration.
                            RunningNotes._Notes[i].Duration = midiStream.GetDuration() + CmdDuration;

                            CmdDuration = 0; // Prevents the note from being added.
                            break;
                        }
                    }
                }

                // Duration == 0 means "no note".
                if ((CmdDuration > 0) && (DstChannel != 0xFF)) {
#ifdef _RCP_VERBOSE
                    ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Note On %02X %02X\n", CmdOffset, CmdType, CmdP0, CmdP1, CmdP2, CmdDuration,
                             midiStream.GetDuration(), Note, CmdP2);
#endif

                    midiStream.WriteEvent(StatusCodes::NoteOn, Note, CmdP2);

                    RunningNotes.Add(midiStream.GetChannel(), Note, 0x80, CmdDuration);
                }
#ifdef _RCP_VERBOSE
                else
                    ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Note On %02X %02X (Skipped)\n", CmdOffset, CmdType, CmdP0, CmdP1, CmdP2, CmdDuration,
                             midiStream.GetDuration(), Note, CmdP2);
#endif
            } else {
                // It's a command.
                switch (CmdType) {
                case 0x90:
                case 0x91:
                case 0x92:
                case 0x93: // send User SysEx (defined via header)
                case 0x94:
                case 0x95:
                case 0x96:
                case 0x97: {
                    if (DstChannel == 0xFF)
                        break;

                    const rcp_user_sysex_t &us = _SysEx[CmdType & 0x07];

                    uint16_t Size = ConvertRCPSysExToMIDISysEx(us.Data, (uint16_t)us.Size, Temp, CmdP1, CmdP2, SrcChannel);

                    // Append 0xF7 byte. It may be omitted with UserSysExs of length 0x18.
                    if ((Size > 0) && Temp[Size - 1] != 0xF7)
                        Temp[Size++] = 0xF7;

#ifdef _RCP_VERBOSE
                    {
                        ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Send User SysEx \"%.*s\",", CmdOffset, CmdType, CmdP0, CmdP1, CmdP2, CmdDuration,
                                 midiStream.GetDuration(), us.Name.Len, us.Name.Data);

                        for (uint16_t i = 0; i < Size; ++i)
                            ::printf(" %02X", Temp[i]);

                        ::putchar('\n');
                    }
#endif

                    if ((us.Name.Len > 0) && _Options._WriteSysExNames)
                        midiStream.WriteMetaEvent(MetaDataTypes::Text, us.Name.Data, us.Name.Len);

                    if (Size > 1)
                        midiStream.WriteEvent(StatusCodes::SysEx, Temp, Size);
#ifdef _RCP_VERBOSE
                    else
                        ::printf("Warning: Track %2u, 0x%04X: Using empty User SysEx command %u.\n", TrackId, CmdOffset, CmdType & 0x07);
#endif
                    break;
                }

                case 0x98: // Send SysEx
                {
                    uint16_t Size = GetMultiCmdDataSize(data, size, Offset, MCMD_INI_EXCLUDE | MCMD_RET_DATASIZE);

                    if (Text.Size < (uint32_t)Size)
                        Text.Grow((uint32_t)((Size + 0x0F) & ~0x0F)); // Round up to 0x10.

                    Size = ReadMultiCmdData(data, size, &Offset, Text.Data, Text.Size, MCMD_INI_EXCLUDE);

                    if (DstChannel == 0xFF)
                        break;

                    Size = ConvertRCPSysExToMIDISysEx(Text.Data, Size, Temp, CmdP1, CmdP2, SrcChannel);

#ifdef _RCP_VERBOSE
                    {
                        ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Send SysEx,", CmdOffset, CmdType, CmdP0, CmdP1, CmdP2, CmdDuration,
                                 midiStream.GetDuration());

                        for (uint16_t i = 0; i < Size; ++i)
                            ::printf(" %02X", Temp[i]);

                        ::putchar('\n');
                    }
#endif

                    midiStream.WriteEvent(StatusCodes::SysEx, Temp, Size);
                    break;
                }

                    // case 0x99: // Execute external command

                case 0xC0: // DX7 Function
                case 0xC1: // DX Parameter
                case 0xC2: // DX RERF
                case 0xC3: // TX Function
                case 0xC5: // FB-01 P Parameter
                case 0xC7: // TX81Z V VCED
                case 0xC8: // TX81Z A ACED
                case 0xC9: // TX81Z P PCED
                case 0xCC: // DX7-2 R Remote SW
                case 0xCD: // DX7-2 A ACED
                case 0xCE: // DX7-2 P PCED
                case 0xCF: // TX802 P PCED
                {
                    if (DstChannel == 0xFF)
                        break;

                    static const uint8_t DXParameters[0x10] = {
                        0x08, 0x00, 0x04, 0x11, 0xFF, 0x15, 0xFF, 0x12, 0x13, 0x10, 0xFF, 0xFF, 0x1B, 0x18, 0x19, 0x1A,
                    };

                    Temp[0] = 0x43; // Yamaha ID
                    Temp[1] = (uint8_t)(0x10 | midiStream.GetChannel());
                    Temp[2] = DXParameters[CmdType & 0x0F];
                    Temp[3] = CmdP1;
                    Temp[4] = CmdP2;
                    Temp[5] = 0xF7;

#ifdef _RCP_VERBOSE
                    {
                        ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Yamaha SysEx", CmdOffset, CmdType, CmdP0, CmdP1, CmdP2, CmdDuration,
                                 midiStream.GetDuration());

                        for (uint16_t i = 0; i < 6; ++i)
                            ::printf(" %02X", Temp[i]);

                        ::putchar('\n');
                    }
#endif

                    midiStream.WriteEvent(StatusCodes::SysEx, Temp, 6);
                    break;
                }

                case 0xC6: // FB-01 S System
                {
                    if (DstChannel == 0xFF)
                        break;

                    Temp[0] = 0x43; // Yamaha ID
                    Temp[1] = 0x75;
                    Temp[2] = midiStream.GetChannel();
                    Temp[3] = 0x10;
                    Temp[4] = CmdP1;
                    Temp[5] = CmdP2;
                    Temp[6] = 0xF7;

#ifdef _RCP_VERBOSE
                    {
                        ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Yamaha SysEx", CmdOffset, CmdType, CmdP0, CmdP1, CmdP2, CmdDuration,
                                 midiStream.GetDuration());

                        for (uint16_t i = 0; i < 6; ++i)
                            ::printf(" %02X", Temp[i]);

                        ::putchar('\n');
                    }
#endif

                    midiStream.WriteEvent(StatusCodes::SysEx, Temp, 7);
                    break;
                }

                case 0xCA: // TX81Z S System
                case 0xCB: // TX81Z E EFFECT
                {
                    if (DstChannel == 0xFF)
                        break;

                    Temp[0] = 0x43; // Yamaha ID
                    Temp[1] = (uint8_t)(0x10 | midiStream.GetChannel());
                    Temp[2] = 0x10;
                    Temp[3] = (uint8_t)(0x7B + (CmdType - 0xCA)); // command CA -> param = 7B, command CB -> param = 7C
                    Temp[4] = CmdP1;
                    Temp[5] = CmdP2;
                    Temp[6] = 0xF7;

#ifdef _RCP_VERBOSE
                    {
                        ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Yamaha SysEx", CmdOffset, CmdType, CmdP0, CmdP1, CmdP2, CmdDuration,
                                 midiStream.GetDuration());

                        for (uint16_t i = 0; i < 6; ++i)
                            ::printf(" %02X", Temp[i]);

                        ::putchar('\n');
                    }
#endif

                    midiStream.WriteEvent(StatusCodes::SysEx, Temp, 7);
                    break;
                }

                case 0xD0: // Yamaha Base Address
                {
                    XGParameters[2] = CmdP1;
                    XGParameters[3] = CmdP2;

#ifdef _RCP_VERBOSE
                    ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Yamaha XG base address %02X %02X", CmdOffset, CmdType, CmdP0, CmdP1, CmdP2,
                             CmdDuration, midiStream.GetDuration(), CmdP1, CmdP2);
#endif
                    break;
                }

                case 0xD1: // Yamaha Device Data
                {
                    XGParameters[0] = CmdP1;
                    XGParameters[1] = CmdP2;

#ifdef _RCP_VERBOSE
                    ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Yamaha XG device data %02X %02X", CmdOffset, CmdType, CmdP0, CmdP1, CmdP2, CmdDuration,
                             midiStream.GetDuration(), CmdP1, CmdP2);
#endif
                    break;
                }

                case 0xD2: // Yamaha Address / Parameter
                {
                    XGParameters[4] = CmdP1;
                    XGParameters[5] = CmdP2;

                    if (DstChannel == 0xFF)
                        break;

                    Temp[0] = 0x43; // Yamaha ID
                    ::memcpy(Temp + 1, &XGParameters[0], 6);
                    Temp[7] = 0xF7;

#ifdef _RCP_VERBOSE
                    {
                        ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Yamaha XG SysEx", CmdOffset, CmdType, CmdP0, CmdP1, CmdP2, CmdDuration,
                                 midiStream.GetDuration());

                        for (uint16_t i = 0; i < 8; ++i)
                            ::printf(" %02X", Temp[i]);

                        ::putchar('\n');
                    }
#endif

                    midiStream.WriteEvent(StatusCodes::SysEx, Temp, 8);
                    break;
                }

                case 0xD3: // Yamaha XG Address / Parameter
                {
                    XGParameters[4] = CmdP1;
                    XGParameters[5] = CmdP2;

                    if (DstChannel == 0xFF)
                        break;

                    Temp[0] = 0x43; // Yamaha ID
                    Temp[1] = 0x10; // Parameter Change
                    Temp[2] = 0x4C; // XG
                    ::memcpy(Temp + 3, &XGParameters[2], 4);
                    Temp[7] = 0xF7;

#ifdef _RCP_VERBOSE
                    {
                        ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Yamaha XG SysEx", CmdOffset, CmdType, CmdP0, CmdP1, CmdP2, CmdDuration,
                                 midiStream.GetDuration());

                        for (uint16_t i = 0; i < 8; ++i)
                            ::printf(" %02X", Temp[i]);

                        ::putchar('\n');
                    }
#endif

                    midiStream.WriteEvent(StatusCodes::SysEx, Temp, 8);
                    break;
                }

                case 0xDC: // MKS-7
                {
                    if (DstChannel == 0xFF)
                        break;

                    Temp[0] = 0x41; // Roland ID
                    Temp[1] = 0x32;
                    Temp[2] = midiStream.GetChannel();
                    Temp[3] = CmdP1;
                    Temp[4] = CmdP2;
                    Temp[5] = 0xF7;

#ifdef _RCP_VERBOSE
                    {
                        ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Roland MKS-7 SysEx", CmdOffset, CmdType, CmdP0, CmdP1, CmdP2, CmdDuration,
                                 midiStream.GetDuration());

                        for (uint16_t i = 0; i < 6; ++i)
                            ::printf(" %02X", Temp[i]);

                        ::putchar('\n');
                    }
#endif

                    midiStream.WriteEvent(StatusCodes::SysEx, Temp, 6);
                    break;
                }

                case 0xDD: // Roland Base Address
                {
                    GSParameters[2] = CmdP1;
                    GSParameters[3] = CmdP2;

#ifdef _RCP_VERBOSE
                    ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Roland GS base address %02X %02X\n", CmdOffset, CmdType, CmdP0, CmdP1, CmdP2,
                             CmdDuration, midiStream.GetDuration(), CmdP1, CmdP2);
#endif
                    break;
                }

                case 0xDE: // Roland Parameter
                {
                    GSParameters[4] = CmdP1;
                    GSParameters[5] = CmdP2;

                    if (DstChannel == 0xFF)
                        break;

                    Temp[0] = 0x41; // Roland ID
                    Temp[1] = GSParameters[0];
                    Temp[2] = GSParameters[1];
                    Temp[3] = 0x12;

                    uint8_t CheckSum = 0;

                    for (uint8_t i = 0; i < 4; ++i) {
                        Temp[4 + i] = GSParameters[2 + i];
                        CheckSum += GSParameters[2 + i];
                    }

                    Temp[8] = (uint8_t)((0x100 - CheckSum) & 0x7F);
                    Temp[9] = 0xF7;

#ifdef _RCP_VERBOSE
                    {
                        ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Roland GS SysEx", CmdOffset, CmdType, CmdP0, CmdP1, CmdP2, CmdDuration,
                                 midiStream.GetDuration());

                        for (uint16_t i = 0; i < 10; ++i)
                            ::printf(" %02X", Temp[i]);

                        ::putchar('\n');
                    }
#endif

                    midiStream.WriteEvent(StatusCodes::SysEx, Temp, 10);
                    break;
                }

                case 0xDF: // Roland Device
                {
                    GSParameters[0] = CmdP1;
                    GSParameters[1] = CmdP2;

#ifdef _RCP_VERBOSE
                    ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Roland GS device %02X %02X\n", CmdOffset, CmdType, CmdP0, CmdP1, CmdP2, CmdDuration,
                             midiStream.GetDuration(), CmdP1, CmdP2);
#endif
                    break;
                }

                case 0xE1: // Set XG instrument
                {
                    if (DstChannel == 0xFF)
                        break;

#ifdef _RCP_VERBOSE
                    ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Set XG instrument (Control Change 20 %02X, Program Change %02X 00)\n", CmdOffset,
                             CmdType, CmdP0, CmdP1, CmdP2, CmdDuration, midiStream.GetDuration(), CmdP2, CmdP1);
#endif

                    midiStream.WriteEvent(StatusCodes::ControlChange, 0x20, CmdP2);
                    midiStream.WriteEvent(StatusCodes::ProgramChange, CmdP1, 0x00);
                    break;
                }

                case 0xE2: // Set GS instrument
                {
                    if (DstChannel == 0xFF)
                        break;

#ifdef _RCP_VERBOSE
                    ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Set GS instrument (Control Change 20 %02X, Program Change %02X 00)\n", CmdOffset,
                             CmdType, CmdP0, CmdP1, CmdP2, CmdDuration, midiStream.GetDuration(), CmdP2, CmdP1);
#endif

                    midiStream.WriteEvent(StatusCodes::ControlChange, 0x00, CmdP2);
                    midiStream.WriteEvent(StatusCodes::ProgramChange, CmdP1, 0x00);
                    break;
                }

                case 0xE5: // Key Scan
                {
                    ::printf("Warning: Track %2u, 0x%04X: Key Scan command found.\n", TrackId, CmdOffset);
                    break;
                }

                case 0xE6: // MIDI channel
                {
                    CmdP1--; // It's same as in the track header, except 1 added.

                    if (CmdP1 & 0x80) {
                        // Ignore the message when the KeepMutedChannels option is off. Else set midiDev to 0xFF to prevent events from being written.
                        if (!_Options._KeepDummyChannels) {
                            DstChannel = 0xFF;
                            SrcChannel = 0x00;
                        }
                    } else {
                        DstChannel = (uint8_t)(CmdP1 >> 4);   // port ID
                        SrcChannel = (uint8_t)(CmdP1 & 0x0F); // channel ID

#ifdef _RCP_VERBOSE
                        ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Set MIDI channel (MIDI Port %02X, Channel Prefix %02X)\n", CmdOffset, CmdType,
                                 CmdP0, CmdP1, CmdP2, CmdDuration, midiStream.GetDuration(), DstChannel, SrcChannel);
#endif

                        midiStream.WriteMetaEvent(MetaDataTypes::MIDIPort, &DstChannel, 1);
                        midiStream.WriteMetaEvent(MetaDataTypes::ChannelPrefix, &SrcChannel, 1);
                    }

                    midiStream.SetChannel(SrcChannel);
                    break;
                }

                case 0xE7: // Tempo Modifier, P1 = multiplicator (20h = 50%, 40h = 100%, 80h = 200%) + P2 = 0 / Else: Just set tempo, 01..FF - interpolate tempo
                           // over P2 ticks.
                {
#ifdef _RCP_VERBOSE
                    if (CmdP2 != 0)
                        ::printf("Warning: Track %2u, 0x%04X: Ignoring gradual tempo change. Speed 0x40, P2 = 0x%02X.\n", TrackId, CmdOffset, CmdP2);
#endif

                    if (CmdP1 == 0)
                        CmdP1 = 64;

                    uint32_t Ticks = BPM2Ticks(_Tempo, CmdP1);

#ifdef _RCP_VERBOSE
                    ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Set tempo to %u bpm / %u ticks.\n", CmdOffset, CmdType, CmdP0, CmdP1, CmdP2,
                             CmdDuration, midiStream.GetDuration(), _Tempo, Ticks);
#endif

                    midiStream.WriteMetaEvent(MetaDataTypes::SetTempo, Ticks, 3u);
                    break;
                }

                case 0xEA: // Channel Pressure
                {
                    if (DstChannel == 0xFF)
                        break;

#ifdef _RCP_VERBOSE
                    ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Channel Pressure %02X\n", CmdOffset, CmdType, CmdP0, CmdP1, CmdP2, CmdDuration,
                             midiStream.GetDuration(), CmdP1);
#endif

                    midiStream.WriteEvent(StatusCodes::ChannelPressure, CmdP1, 0);
                    break;
                }

                case 0xEB: // Control Change
                {
                    if (DstChannel == 0xFF)
                        break;

#ifdef _RCP_VERBOSE
                    ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Control Change %02X %02X\n", CmdOffset, CmdType, CmdP0, CmdP1, CmdP2, CmdDuration,
                             midiStream.GetDuration(), CmdP1, CmdP2);
#endif

                    midiStream.WriteEvent(StatusCodes::ControlChange, CmdP1, CmdP2);
                    break;
                }

                case 0xEC: // Instrument
                {
                    if (DstChannel == 0xFF)
                        break;

                    if (CmdP1 < 0x80) {
#ifdef _RCP_VERBOSE
                        ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Program Change %02X\n", CmdOffset, CmdType, CmdP0, CmdP1, CmdP2, CmdDuration,
                                 midiStream.GetDuration(), CmdP1);
#endif

                        midiStream.WriteEvent(StatusCodes::ProgramChange, CmdP1, 0);
                    } else if ((CmdP1 < 0xC0) && (SrcChannel >= 1 && SrcChannel < 9)) {
#ifdef _RCP_VERBOSE
                        ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: MT-32 instrument change\n", CmdOffset, CmdType, CmdP0, CmdP1, CmdP2, CmdDuration,
                                 midiStream.GetDuration());
#endif

                        // Set MT-32 instrument from user bank used by RCP files from Granada X68000.
                        uint8_t PartMemOffset = (uint8_t)((SrcChannel - 1) << 4);

                        ::memcpy(Temp, MT32PatchChange, 0x07);

                        Temp[0x00] = (uint8_t)((CmdP1 >> 6) & 0x03);
                        Temp[0x01] = (uint8_t)((CmdP1 >> 0) & 0x3F);

                        midiStream.WriteRolandSysEx(SysExHeaderMT32, (uint32_t)(0x030000 | PartMemOffset), Temp, 7, 0);
                    }
                    break;
                }

                case 0xED: // Note Aftertouch
                {
                    if (DstChannel == 0xFF)
                        break;

#ifdef _RCP_VERBOSE
                    ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Key Pressure %02X %02X\n", CmdOffset, CmdType, CmdP0, CmdP1, CmdP2, CmdDuration,
                             midiStream.GetDuration(), CmdP1, CmdP2);
#endif

                    midiStream.WriteEvent(StatusCodes::KeyPressure, CmdP1, CmdP2);
                    break;
                }

                case 0xEE: // Pitch Bend
                {
                    if (DstChannel == 0xFF)
                        break;

#ifdef _RCP_VERBOSE
                    ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Pitch Bend Change %02X %02X\n", CmdOffset, CmdType, CmdP0, CmdP1, CmdP2, CmdDuration,
                             midiStream.GetDuration(), CmdP1, CmdP2);
#endif

                    midiStream.WriteEvent(StatusCodes::PitchBendChange, CmdP1, CmdP2);
                    break;
                }

                case 0xF5: // Key Signature Change
                {
                    RCP2MIDIKeySignature((uint8_t)CmdP0, Temp);

#ifdef _RCP_VERBOSE
                    ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Key Signature %02X %02X\n", CmdOffset, CmdType, CmdP0, CmdP1, CmdP2, CmdDuration,
                             midiStream.GetDuration(), Temp[0], Temp[1]);
#endif

                    midiStream.WriteMetaEvent(MetaDataTypes::KeySignature, Temp, 2);

                    CmdP0 = 0;
                    break;
                }

                case 0xF6: // Comment
                {
                    uint16_t Size = GetMultiCmdDataSize(data, size, Offset, MCMD_INI_INCLUDE | MCMD_RET_DATASIZE);

                    if (Text.Size < Size)
                        Text.Grow((uint8_t)((Size + 0x0F) & ~0x0F)); // Round up to 0x10.

                    Size = ReadMultiCmdData(data, size, &Offset, Text.Data, Text.Size, MCMD_INI_INCLUDE);
                    Size = GetTrimmedLength((char *)Text.Data, Size, ' ', false);

#ifdef _RCP_VERBOSE
                    ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Meta Data Text \"%s\"\n", CmdOffset, CmdType, CmdP0, CmdP1, CmdP2, CmdDuration,
                             midiStream.GetDuration(), TextToUTF8((const char *)Text.Data, Size).c_str());
#endif

                    midiStream.WriteMetaEvent(MetaDataTypes::Text, Text.Data, Size);

                    CmdP0 = 0;
                    break;
                }

                case 0xF7: // Continuation of previous command
                {
#ifdef _RCP_VERBOSE
                    ::printf("Warning: Track %2u, 0x%04X: Unexpected continuation command.\n", TrackId, CmdOffset);
#endif
                    break;
                }

                case 0xF8: // Loop End
                {
                    if (LoopCount != 0) {
                        LoopCount--;

                        LoopCounter[LoopCount]++;

                        bool TakeLoop = false;

                        // Loops == 0 -> infinite, but some songs also use very high values (like 0xFF) for that.
                        if (CmdP0 == 0 || CmdP0 >= 0x7F) {
                            // Infinite loop
                            if (LoopCounter[LoopCount] < 0x80 && (DstChannel != 0xFF)) {
#ifdef _RCP_VERBOSE
                                ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Loop Begin (RPG Maker)\n", CmdOffset, CmdType, CmdP0, CmdP1, CmdP2,
                                         CmdDuration, midiStream.GetDuration());
#endif

                                midiStream.WriteEvent(StatusCodes::ControlChange, 0x6F, (uint8_t)LoopCounter[LoopCount]);
                            }

                            if (LoopCounter[LoopCount] < track->LoopCount)
                                TakeLoop = true;
                        } else {
                            if (LoopCounter[LoopCount] < CmdP0)
                                TakeLoop = true;
                        }

                        if (_Options._WriteBarMarkers) {
                            int Length = ::sprintf_safe((char *)Temp, _countof(Temp), "Loop %u End (%u/%u)", 1 + LoopCount, LoopCounter[LoopCount], CmdP0);

                            midiStream.WriteMetaEvent(MetaDataTypes::CueMarker, Temp, (uint32_t)Length);
                        }

                        if (TakeLoop) {
                            ParentOffs = LoopParentOffs[LoopCount];
                            Offset = LoopStartOffs[LoopCount];

                            LoopCount++;
                        }
                    } else {
#ifdef _RCP_VERBOSE
                        ::printf("Warning: Track %2u, 0x%04X: Loop End without Loop Start.\n", TrackId, CmdOffset);
#endif

                        if (_Options._WriteBarMarkers)
                            midiStream.WriteMetaEvent(MetaDataTypes::CueMarker, "Bad Loop End");
                    }

                    CmdP0 = 0;
                    break;
                }

                case 0xF9: // Loop Begin
                {
                    if (_Options._WriteBarMarkers) {
                        int Length = ::sprintf_safe((char *)Temp, _countof(Temp), "Loop %u Start", LoopCount + 1);

                        midiStream.WriteMetaEvent(MetaDataTypes::CueMarker, Temp, (uint32_t)Length);
                    }

                    if (LoopCount < 8) {
                        if (Offset == track->LoopStartOffs && DstChannel != 0xFF) {
#ifdef _RCP_VERBOSE
                            ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Loop Begin (RPG Maker)\n", CmdOffset, CmdType, CmdP0, CmdP1, CmdP2,
                                     CmdDuration, midiStream.GetDuration());
#endif

                            midiStream.WriteEvent(StatusCodes::ControlChange, 0x6F, 0);
                        }

                        LoopParentOffs[LoopCount] = ParentOffs; // required by YS-2･018.RCP
                        LoopStartOffs[LoopCount] = Offset;
                        LoopCounter[LoopCount] = 0;

                        LoopCount++;
                    }
#ifdef _RCP_VERBOSE
                    else
                        ::printf("Error: Track %u, 0x%04X: Trying to do more than 8 nested loops.\n", TrackId, CmdOffset);
#endif

                    CmdP0 = 0;
                    break;
                }

                case 0xFC: // Repeat previous bar
                {
                    // Behaviour of the FC command:
                    // - Already in "repeating bar" mode: return to parent bar (same as FD)
                    // - Else follow chain of FC commands: i.e. "FC -> FC -> FC -> non-FC command" is a valid sequence that is followed to the end.
                    if (ParentOffs == 0) {
                        if (_Version == 2)
                            Offset -= 4;
                        else if (_Version == 3)
                            Offset -= 6;

                        uint16_t BarID = 0;

                        uint32_t RepeatOffs = 0;
                        uint32_t CachedOffs = 0;

                        do {
                            if (_Version == 2) {
                                CmdP0 = data[Offset + 0x01];
                                CmdP1 = data[Offset + 0x02];
                                CmdP2 = data[Offset + 0x03];

                                BarID = (uint16_t)(CmdP0 | ((CmdP1 & 0x03) << 8));
                                RepeatOffs = (uint32_t)((CmdP1 & ~0x03) | (CmdP2 << 8));

                                Offset += 4;
                            } else if (_Version == 3) {
                                CmdP0 = ReadLE16(&data[Offset + 0x02]);
                                CmdDuration = ReadLE16(&data[Offset + 0x04]);
                                BarID = CmdP0;

                                // Why has the first command ID 0x30?
                                RepeatOffs = (uint32_t)(0x002E + (CmdDuration - 0x0030) * 0x06); // Calculate offset from command ID

                                Offset += 6;
                            }

                            if (_Options._WriteBarMarkers) {
                                int Length = ::sprintf_safe((char *)Temp, _countof(Temp), "Repeat Bar %u", 1 + BarID);

                                midiStream.WriteMetaEvent(MetaDataTypes::CueMarker, Temp, (uint32_t)Length);
                            }

                            if (BarID >= BarOffsets.size()) {
#ifdef _RCP_VERBOSE
                                ::printf("Warning: Track %2u, 0x%04X: Trying to repeat invalid bar %u. Max %u bars.\n", TrackId, CmdOffset, BarID,
                                         BarCount + 1);
#endif
                                break;
                            }

                            CachedOffs = BarOffsets[BarID] - TrackHead;

                            //  if (cachedPos != repeatPos)
                            //      printf("Warning: Track %2u: Repeat Measure %u: offset mismatch (file: 0x%04X != expected 0x%04X) at 0x%04X!\n", trkID,
                            //      measureID, repeatPos, cachedPos, prevPos);

                            if (TrackHead + RepeatOffs == CmdOffset)
                                break; // prevent recursion (just for safety)

                            if (!ParentOffs) // necessary for following FC command chain
                                ParentOffs = Offset;

                            RepeatingBarID = BarID;

                            // YS3-25.RCP relies on using the actual offset. (*Some* of its bar numbers are off by 1.)
                            Offset = TrackHead + RepeatOffs;
                            CmdOffset = Offset;
                        } while (data[Offset] == 0xFC);
                    } else {
#ifdef _RCP_VERBOSE
                        ::printf("Warning: Track %2u, 0x%04X: Leaving recursive repeat bar.\n", TrackId, CmdOffset);
#endif
                        Offset = ParentOffs;

                        ParentOffs = 0x00;
                        RepeatingBarID = 0xFFFF;
                    }

                    CmdP0 = 0;
                    break;
                }

                case 0xFD: // Bar End
                {
                    if (BarOffsets.size() >= 0x8000) // Prevent infinite loops
                    {
                        EndOfTrack = true;
                        break;
                    }

                    if (ParentOffs) {
                        Offset = ParentOffs;
                        ParentOffs = 0x00;
                        RepeatingBarID = 0xFFFF;
                    }

                    BarOffsets.push_back(Offset);
                    BarCount++;

                    CmdP0 = 0;

                    if (_Options._WriteBarMarkers) {
                        int Length = ::sprintf_safe((char *)Temp, _countof(Temp), "Bar %u", 1 + BarCount);

                        midiStream.WriteMetaEvent(MetaDataTypes::CueMarker, Temp, (uint32_t)Length);
                    }

                    if (_Options._WolfteamLoopMode && (BarOffsets.size() == 2)) {
                        LoopCount = 0;

                        if (DstChannel != 0xFF)
                            midiStream.WriteEvent(StatusCodes::ControlChange, 0x6F, 0);

                        LoopParentOffs[LoopCount] = ParentOffs;
                        LoopStartOffs[LoopCount] = Offset;
                        LoopCounter[LoopCount] = 0;

                        LoopCount++;
                    }
                    break;
                }

                case 0xFE: // Track end
                {
                    EndOfTrack = true;
                    CmdP0 = 0;

                    if (_Options._WolfteamLoopMode) {
                        LoopCount = 0;
                        LoopCounter[LoopCount]++;

                        if (LoopCounter[LoopCount] < 0x80 && DstChannel != 0xFF) {
                            midiStream.WriteEvent(StatusCodes::ControlChange, 0x6F, (uint8_t)LoopCounter[LoopCount]);

#ifdef _RCP_VERBOSE
                            ::printf("    %04X: %02X %04X %02X %02X %04X | %08X: Loop Begin (RPG Maker)\n", CmdOffset, CmdType, CmdP0, CmdP1, CmdP2,
                                     CmdDuration, midiStream.GetDuration());
#endif
                        }

                        if (LoopCounter[LoopCount] < _Options._RCPLoopCount) {
                            ParentOffs = LoopParentOffs[LoopCount];
                            Offset = LoopStartOffs[LoopCount];
                            LoopCount++;
                            EndOfTrack = false;
                        }
                    }
                    break;
                }

                default:
#ifdef _RCP_VERBOSE
                    ::printf("    %04X: %02X %04X %02X %02X %04X Unknown command\n", CmdOffset, CmdType, CmdP0, CmdP1, CmdP2, CmdDuration);
#endif
                    break;
                }
            }

            {
                uint32_t Timestamp = midiStream.GetDuration() + CmdP0;

                if ((StartTick < 0) && (Timestamp > 0)) {
                    StartTick += Timestamp;

                    if (StartTick >= 0) {
                        Timestamp = (uint32_t)StartTick;
                        StartTick = 0;
                    } else
                        Timestamp = 0;
                }

                midiStream.SetDuration(Timestamp);
            }
        }
    }

    if (DstChannel == 0xFF)
        midiStream.SetDuration(0);

    midiStream.SetDuration(RunningNotes.Flush(midiStream, false));

    *offset = TrackHead + TrackSize;
}

/// <summary>
///
/// </summary>
uint16_t rcp_file_t::GetMultiCmdDataSize(const uint8_t *data, uint32_t size, uint32_t offset, uint8_t flags) const {
    uint16_t Size = (uint16_t)((flags & MCMD_INI_INCLUDE) ? 1 : 0);

    if (_Version == 2) {
        for (uint32_t inPos = offset; inPos < size && data[inPos] == 0xF7; inPos += 0x04)
            Size++;

        if (flags & MCMD_RET_DATASIZE)
            Size *= 2; // 2 data bytes per command
    } else if (_Version == 3) {
        for (uint32_t inPos = offset; inPos < size && data[inPos] == 0xF7; inPos += 0x06)
            Size++;

        if (flags & MCMD_RET_DATASIZE)
            Size *= 5; // 5 data bytes per command
    }

    return Size;
}

/// <summary>
///
/// </summary>
uint16_t rcp_file_t::ReadMultiCmdData(const uint8_t *srcData, uint32_t srcSize, uint32_t *srcOffset, uint8_t *dstData, uint32_t dstSize, uint8_t flags) const {
    uint32_t SrcOffset = *srcOffset;
    uint32_t DstOffset = 0;

    if (_Version == 2) {
        if (flags & MCMD_INI_INCLUDE) {
            if (DstOffset + 0x02 > dstSize)
                return 0x00;

            dstData[DstOffset++] = srcData[SrcOffset - 0x02];
            dstData[DstOffset++] = srcData[SrcOffset - 0x01];
        }

        for (; SrcOffset < srcSize && srcData[SrcOffset] == 0xF7; SrcOffset += 0x04) {
            if (DstOffset + 0x02 > dstSize)
                break;

            dstData[DstOffset++] = srcData[SrcOffset + 0x02];
            dstData[DstOffset++] = srcData[SrcOffset + 0x03];
        }
    } else if (_Version == 3) {
        if (flags & MCMD_INI_INCLUDE) {
            if (DstOffset + 0x05 > dstSize)
                return 0x00;

            ::memcpy(&dstData[DstOffset], &srcData[SrcOffset - 0x05], 0x05);
            DstOffset += 0x05;
        }

        for (; SrcOffset < srcSize && srcData[SrcOffset] == 0xF7; SrcOffset += 0x06) {
            if (DstOffset + 0x05 > dstSize)
                break;

            ::memcpy(&dstData[DstOffset], &srcData[SrcOffset + 0x01], 0x05);
            DstOffset += 0x05;
        }
    }

    *srcOffset = SrcOffset;

    return (uint16_t)DstOffset;
}

/// <summary>
/// Converts the time signature from RCP to MIDI.
/// </summary>
void RCP2MIDITimeSignature(uint8_t numerator, uint8_t denominator, uint8_t data[4]) {
    const uint8_t base2 = DetermineShift(denominator);

    data[0] = numerator;              // numerator
    data[1] = base2;                  // log2(denominator)
    data[2] = (uint8_t)(96 >> base2); // metronome pulse
    data[3] = 8;                      // 32nd notes per 1/4 note
}

/// <summary>
/// Converts the key signature from RCP to MIDI.
/// </summary>
void RCP2MIDIKeySignature(uint8_t keySignature, uint8_t data[2]) {
    data[0] = (uint8_t)((keySignature & 0x08) ? -(keySignature & 0x07) /* flats */ : (keySignature & 0x07)); // sharps
    data[1] = (uint8_t)((keySignature & 0x10) >> 4);                                                         // major (0) / minor (1)
}

/// <summary>
///
/// </summary>
static uint8_t DetermineShift(uint32_t value) {
    uint8_t shift = 0;

    value >>= 1;

    while (value) {
        shift++;
        value >>= 1;
    }

    return shift;
}

/// <summary>
///
/// </summary>
static uint16_t ConvertRCPSysExToMIDISysEx(const uint8_t *srcData, uint16_t srcSize, uint8_t *dstData, uint8_t p1, uint8_t p2, uint8_t channel) {
    uint8_t Checksum = 0;

    uint16_t n = 0;

    for (uint16_t i = 0; i < srcSize; ++i) {
        uint8_t Data = srcData[i];

        if (Data & 0x80) {
            switch (Data) {
            case 0x80:
                Data = p1;
                break;

            case 0x81:
                Data = p2;
                break;

            case 0x82:
                Data = channel;
                break;

            case 0x83: // Initialize Roland cChecksum.
                Checksum = 0;
                break;

            case 0x84: // Store Roland checksum.
                Data = (uint8_t)((0x100 - Checksum) & 0x7F);
                break;

            case 0xF7: // SysEx end
                dstData[n++] = Data;

                return n;

            default:
#ifdef _RCP_VERBOSE
                ::printf("Warning: Unknown SysEx command 0x%02X found in SysEx data.\n", Data);
#endif
                break;
            }
        }

        if (!(Data & 0x80)) {
            dstData[n++] = Data;

            Checksum += Data;
        }
    }

    return n;
}
