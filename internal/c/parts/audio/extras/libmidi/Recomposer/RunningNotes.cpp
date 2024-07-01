
/** $VER: RunningNotes.cpp (2024.05.09) P. Stuer - Based on Valley Bell's rpc2mid (https://github.com/ValleyBell/MidiConverters). **/

#include "RunningNotes.h"

// Adds a note event to the "runNotes" list, so that NoteOff events can be inserted automatically by Check() while processing delays.
// "length" specifies the number of ticks after which the note is turned off.
// "velOff" specifies the velocity for the Note Off event. A value of 0x80 results in Note On with velocity 0.
// Returns a pointer to the inserted struct or NULL if (NoteCnt >= NoteMax).
void running_notes_t::Add(uint8_t channel, uint8_t note, uint8_t velocityOff, uint32_t duration) {
    if (_Count >= MAX_RUN_NOTES)
        return;

    running_note_t &rn = _Notes[_Count];

    rn.Channel = channel;
    rn.Note = note;
    rn.NoteOffVelocity = velocityOff;
    rn.Duration = duration;

    _Count++;
}

// Checks, if any note expires within the N ticks specified by the "duration" parameter and
// insert respective Note Off events. In that case "duration" will be reduced.
// Call this function from the delay handler and before extending notes.
// Returns the number of expired notes.
size_t running_notes_t::Check(midi_stream_t &midiStream, uint32_t &duration) {
    size_t ExpiredNotes = 0;

    while (_Count > 0) {
        uint32_t NewDuration = duration + 1;

        // 1. Check if we're going beyond a note's timeout.
        for (size_t i = 0; i < _Count; ++i) {
            running_note_t &n = _Notes[i];

            if (n.Duration < NewDuration)
                NewDuration = n.Duration;
        }

        if (NewDuration > duration)
            break; // The note is still playing. Continue processing the event.

        // 2. Advance all notes by X ticks.
        for (size_t i = 0; i < _Count; ++i)
            _Notes[i].Duration -= NewDuration;

        duration -= NewDuration;

        // 3. Send NoteOff for expired notes.
        for (size_t i = 0; i < _Count; ++i) {
            running_note_t &n = _Notes[i];

            if (n.Duration > 0)
                continue;

            {
                midiStream.WriteVariableLengthQuantity(NewDuration);

                NewDuration = 0;

                midiStream.Ensure(3);

                if (n.NoteOffVelocity < 0x80) {
                    midiStream.Add((uint8_t)(StatusCodes::NoteOff | n.Channel));
                    midiStream.Add(n.Note);
                    midiStream.Add(n.NoteOffVelocity);
                } else {
                    midiStream.Add((uint8_t)(StatusCodes::NoteOn | n.Channel));
                    midiStream.Add(n.Note);
                    midiStream.Add(0);
                }
            }

            _Count--;
            ::memmove(&n, &_Notes[(size_t)i + 1], ((size_t)_Count - i) * sizeof(running_note_t));
            i--;
            ExpiredNotes++;
        }
    }

    return ExpiredNotes;
}

// Writes Note Off events for all running notes.
// "cutNotes" = false -> all notes are played fully (even if "delay" is smaller than the longest note)
// "cutNotes" = true -> notes playing after "delay" ticks are cut there
uint32_t running_notes_t::Flush(midi_stream_t &midiStream, bool shorten) {
    uint32_t Duration = midiStream.GetDuration();

    for (uint16_t i = 0; i < _Count; ++i) {
        if (_Notes[i].Duration > Duration) {
            if (shorten)
                _Notes[i].Duration = Duration; // Cut all notes at timestamp.
            else
                Duration = _Notes[i].Duration; // Remember the highest timestamp.
        }
    }

    Check(midiStream, Duration);

    return Duration;
}
