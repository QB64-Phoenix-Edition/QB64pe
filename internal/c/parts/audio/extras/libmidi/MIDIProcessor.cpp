
/** $VER: MIDIProcessor.cpp (2024.05.19) **/

#include "framework.h"

#include "MIDIProcessor.h"
#include "Recomposer/Support.h"

#include <filesystem>

midi_processor_options_t midi_processor_t::_Options;

const uint8_t midi_processor_t::MIDIEventEndOfTrack[2] = {StatusCodes::MetaData, MetaDataTypes::EndOfTrack};
const uint8_t midi_processor_t::LoopBeginMarker[11] = {StatusCodes::MetaData, MetaDataTypes::POIMarker, 'l', 'o', 'o', 'p', 'S', 't', 'a', 'r', 't'};
const uint8_t midi_processor_t::LoopEndMarker[9] = {StatusCodes::MetaData, MetaDataTypes::POIMarker, 'l', 'o', 'o', 'p', 'E', 'n', 'd'};

/// <summary>
/// Processes a stream of bytes.
/// </summary>
bool midi_processor_t::Process(std::vector<uint8_t> const &data, const char *filePath, midi_container_t &container, const midi_processor_options_t &options) {
    _Options = options;

    const char *FileExtension = (filePath != nullptr) ? GetFileExtension(filePath) : "";

    if (IsSMF(data))
        return ProcessSMF(data, container);

    // .RMI
    if (IsRIFF(data))
        return ProcessRIFF(data, container);

    // .XMI, .XFM
    if (IsXMI(data))
        return ProcessXMI(data, container);

    if (IsMDS(data))
        return ProcessMDS(data, container);

    if (IsHMP(data))
        return ProcessHMP(data, container);

    if (IsHMI(data))
        return ProcessHMI(data, container);

    if (IsMUS(data))
        return ProcessMUS(data, container);

    if (IsLDS(data, FileExtension))
        return ProcessLDS(data, container);

    if (IsGMF(data))
        return ProcessGMF(data, container);

    if (IsRCP(data, FileExtension))
        return ProcessRCP(data, filePath, container);

    if (IsSysEx(data))
        return ProcessSysEx(data, container);

    return false;
}

/// <summary>
/// Returns true if the data represents a SysEx message.
/// </summary>
bool midi_processor_t::IsSysEx(std::vector<uint8_t> const &data) {
    if (data.size() < 2)
        return false;

    if (data[0] != StatusCodes::SysEx || data[data.size() - 1] != StatusCodes::SysExEnd)
        return false;

    return true;
}

/// <summary>
/// Processes a byte stream containing 1 or more SysEx messages.
/// </summary>
bool midi_processor_t::ProcessSysEx(std::vector<uint8_t> const &data, midi_container_t &container) {
    const size_t Size = data.size();

    size_t Index = 0;

    container.Initialize(0, 1);

    midi_track_t Track;

    while (Index < Size) {
        size_t MessageLength = 1;

        if (data[Index] != StatusCodes::SysEx)
            return false;

        while (data[Index + MessageLength++] != StatusCodes::SysExEnd)
            ;

        Track.AddEvent(midi_event_t(0, midi_event_t::Extended, 0, &data[Index], MessageLength));

        Index += MessageLength;
    }

    container.AddTrack(Track);

    return true;
}

/// <summary>
/// Decodes a variable-length quantity.
/// </summary>
int midi_processor_t::DecodeVariableLengthQuantity(std::vector<uint8_t>::const_iterator &data, std::vector<uint8_t>::const_iterator tail) noexcept {
    int Quantity = 0;

    uint8_t Byte;

    do {
        if (data == tail)
            return 0;

        Byte = *data++;
        Quantity = (Quantity << 7) + (Byte & 0x7F);
    } while (Byte & 0x80);

    return Quantity;
}
