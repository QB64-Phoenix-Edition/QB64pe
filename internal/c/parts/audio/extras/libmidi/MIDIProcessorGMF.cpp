
/** $VER: MIDIProcessorGMF.cpp (2023.08.14) Game Music Format (http://www.vgmpf.com/Wiki/index.php?title=GMF) **/

#include "framework.h"

#include "MIDIProcessor.h"

bool midi_processor_t::IsGMF(std::vector<uint8_t> const &data) {
    if (data.size() < 32)
        return false;

    if (data[0] != 'G' || data[1] != 'M' || data[2] != 'F' || data[3] != 1)
        return false;

    return true;
}

bool midi_processor_t::ProcessGMF(std::vector<uint8_t> const &data, midi_container_t &container) {
    uint8_t Data[10];

    container.Initialize(0, 0xC0);

    uint16_t Tempo = (uint16_t)(((uint16_t)data[4] << 8) | data[5]);
    uint32_t ScaledTempo = (uint32_t)Tempo * 100000;

    midi_track_t Track;

    Data[0] = StatusCodes::MetaData;
    Data[1] = MetaDataTypes::SetTempo;
    Data[2] = (uint8_t)(ScaledTempo >> 16);
    Data[3] = (uint8_t)(ScaledTempo >> 8);
    Data[4] = (uint8_t)ScaledTempo;

    Track.AddEvent(midi_event_t(0, midi_event_t::Extended, 0, Data, 5));

    Data[0] = StatusCodes::SysEx;
    Data[1] = 0x41;
    Data[2] = 0x10;
    Data[3] = 0x16;
    Data[4] = 0x12;
    Data[5] = 0x7F;
    Data[6] = 0x00;
    Data[7] = 0x00;
    Data[8] = 0x01;
    Data[9] = StatusCodes::SysExEnd;

    Track.AddEvent(midi_event_t(0, midi_event_t::Extended, 0, Data, 10));

    Data[0] = StatusCodes::MetaData;
    Data[1] = MetaDataTypes::EndOfTrack;

    Track.AddEvent(midi_event_t(0, midi_event_t::Extended, 0, Data, 2));

    container.AddTrack(Track);

    auto it = data.begin() + 7;

    return ProcessSMFTrack(it, data.end(), container, false);
}
