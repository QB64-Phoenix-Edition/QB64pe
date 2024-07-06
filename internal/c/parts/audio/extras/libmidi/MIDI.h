
/** $VER: MIDI.h (2024.05.10) **/

#pragma once

enum StatusCodes {
    NoteOff = 0x80,
    NoteOn = 0x90,

    KeyPressure = 0xA0, // Polyphonic Key Pressure (Aftertouch)
    ControlChange = 0xB0,
    ProgramChange = 0xC0,
    ChannelPressure = 0xD0,
    PitchBendChange = 0xE0,

    SysEx = 0xF0,
    MIDITimeCodeQtrFrame = 0xF1,
    SongPositionPointer = 0xF2,
    SongSelect = 0xF3,

    TuneRequest = 0xF6,
    SysExEnd = 0xF7,
    TimingClock = 0xF8,

    Start = 0xFA,
    Continue = 0xFB,
    Stop = 0xFC,

    ActiveSensing = 0xFE,
    MetaData = 0xFF
};

enum ControlChangeNumbers {
    BankSelect = 0x00,

    LSB = 0x20, // LSB for Control Changes 0 to 31
};

enum MetaDataTypes {
    SequenceNumber = 0x00, // Sequence number in type 0 and 1 MIDI files; pattern number in type 2 MIDI files. (0..65535, default 0, occurs at delta time 0)
    Text = 0x01,           // General "Text" Meta Message. Can be used for any text based data. (string)
    Copyright = 0x02,      // Provides information about a MIDI file�s copyright. (string, occurs at delta time 0 in the first track)
    TrackName = 0x03,      // Track name (string, occurs at delta time 0)
    InstrumentName = 0x04, // Instrument name (string)
    Lyrics = 0x05,         // Stores the lyrics of a song. Typically one syllable per Meta Message. (string)
    POIMarker = 0x06,      // Marks a point of interest in a MIDI file. Can be used as the marker for the beginning of a verse, solo, etc. (string)
    CueMarker = 0x07,      // Marks a cue. IE: �Cue performer 1�, etc (string)

    DeviceName = 0x09, // Gives the name of the device. (string)

    ChannelPrefix = 0x20, // Gives the prefix for the channel on which events are played. (0..255, default 0)
    MIDIPort = 0x21,      // Gives the MIDI Port on which events are played. (0..255, default 0)

    EndOfTrack = 0x2F, // An empty Meta Message that marks the end of a track. Occurs at the end of each track.

    SetTempo = 0x51, // Tempo is in microseconds per beat (quarter note). (0..16777215, default 500000). Occurs anywhere but usually in the first track.

    SMPTEOffset = 0x54, // SMPTE time to denote playback offset from the beginning. Occurs at the beginning of a track and in the first track of files with MIDI
                        // format type 1.

    TimeSignature = 0x58, //
    KeySignature = 0x59,  // Valid values: A A#m Ab Abm Am B Bb Bbm Bm C C# C#m Cb Cm D D#m Db Dm E Eb Ebm Em F F# F#m Fm G G#m Gb Gm

    SequencerSpecific = 0x7F // An unprocessed sequencer specific message containing raw data.
};
