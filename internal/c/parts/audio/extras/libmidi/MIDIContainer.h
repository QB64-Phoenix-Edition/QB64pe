
/** $VER: MIDIContainer.h (2024.06.09) **/

#pragma once

#include "framework.h"

#include "MIDI.h"
#include "Range.h"

struct midi_event_t {
    enum event_type_t {
        NoteOff = 0,     // x080
        NoteOn,          // x090
        KeyPressure,     // x0A0
        ControlChange,   // x0B0
        ProgramChange,   // x0C0
        ChannelPressure, // x0D0
        PitchBendChange, // x0E0
        Extended         // x0F0
    };

    uint32_t Time; // Absolute time
    event_type_t Type;
    uint32_t ChannelNumber;
    std::vector<uint8_t> Data;

    midi_event_t() noexcept : Time(), Type(event_type_t::NoteOff), ChannelNumber() {}

    midi_event_t(const midi_event_t &other) noexcept {
        Time = other.Time;
        Type = other.Type;
        ChannelNumber = other.ChannelNumber;
        Data = other.Data;
    }

    midi_event_t &operator=(const midi_event_t &other) noexcept {
        Time = other.Time;
        Type = other.Type;
        ChannelNumber = other.ChannelNumber;
        Data = other.Data;

        return *this;
    }

    midi_event_t(uint32_t time, event_type_t eventType, uint32_t channelNumber, const uint8_t *data, size_t size) noexcept {
        Time = time;
        Type = eventType;
        ChannelNumber = channelNumber;
        Data.assign(data, data + size);
    }
};

class midi_track_t {
  public:
    midi_track_t() noexcept {}

    midi_track_t(const midi_track_t &track) noexcept {
        _Events = track._Events;
    }

    midi_track_t &operator=(const midi_track_t &track) {
        _Events = track._Events;

        return *this;
    }

    void AddEvent(const midi_event_t &event);
    void RemoveEvent(size_t index);

    size_t GetLength() const noexcept {
        return _Events.size();
    }

    const midi_event_t &operator[](size_t index) const noexcept {
        return _Events[index];
    }

    midi_event_t &operator[](std::size_t index) noexcept {
        return _Events[index];
    }

  public:
    using midi_events_t = std::vector<midi_event_t>;

    using iterator = midi_events_t::iterator;
    using const_iterator = midi_events_t::const_iterator;

    iterator begin() {
        return _Events.begin();
    }

    iterator end() {
        return _Events.end();
    }

    const_iterator begin() const {
        return _Events.begin();
    }

    const_iterator end() const {
        return _Events.end();
    }

    const_iterator cbegin() const {
        return _Events.cbegin();
    }

    const_iterator cend() const {
        return _Events.cend();
    }

    const midi_event_t &front() const noexcept {
        return _Events.front();
    }

    const midi_event_t &back() const noexcept {
        return _Events.back();
    }

  private:
    std::vector<midi_event_t> _Events;
};

struct tempo_item_t {
    uint32_t Time;
    uint32_t Tempo;

    tempo_item_t() noexcept : Time(0), Tempo(0) {}

    tempo_item_t(uint32_t timestamp, uint32_t tempo);
};

class tempo_map_t {
  public:
    void Add(uint32_t tempo, uint32_t timestamp);
    uint32_t TimestampToMS(uint32_t timestamp, uint32_t division) const;

    size_t Size() const {
        return _Items.size();
    }

    const tempo_item_t &operator[](std::size_t p_index) const {
        return _Items[p_index];
    }

    tempo_item_t &operator[](size_t index) {
        return _Items[index];
    }

  private:
    std::vector<tempo_item_t> _Items;
};

struct sysex_item_t {
    size_t Offset;
    size_t Size;
    uint8_t PortNumber;

    sysex_item_t() noexcept : Offset(0), Size(0), PortNumber(0) {}

    sysex_item_t(const sysex_item_t &other);
    sysex_item_t(uint8_t portNumber, std::size_t offset, std::size_t size);
};

class sysex_table_t {
  public:
    size_t AddItem(const uint8_t *data, std::size_t size, uint8_t portNumber);
    bool GetItem(size_t index, const uint8_t *&data, std::size_t &size, uint8_t &portNumber) const;

    size_t Size() const {
        return _Items.size();
    }

  private:
    std::vector<sysex_item_t> _Items;
    std::vector<uint8_t> _Data;
};

struct midi_metadata_item_t {
    uint32_t Timestamp;
    std::string Name;
    std::string Value;

    midi_metadata_item_t() noexcept : Timestamp(0) {}

    midi_metadata_item_t(const midi_metadata_item_t &item) noexcept {
        operator=(item);
    };

    midi_metadata_item_t &operator=(const midi_metadata_item_t &other) noexcept {
        Timestamp = other.Timestamp;
        Name = other.Name;
        Value = other.Value;

        return *this;
    }

    midi_metadata_item_t(midi_metadata_item_t &&item) {
        operator=(item);
    }

    midi_metadata_item_t &operator=(midi_metadata_item_t &&other) {
        Timestamp = other.Timestamp;
        Name = std::move(Name);
        Value = std::move(Value);

        return *this;
    }

    virtual ~midi_metadata_item_t() {}

    midi_metadata_item_t(uint32_t timestamp, const char *name, const char *value) noexcept {
        Timestamp = timestamp;
        Name = name;
        Value = value;
    }
};

class midi_metadata_table_t {
  public:
    midi_metadata_table_t() noexcept {}

    void AddItem(const midi_metadata_item_t &item);
    void Append(const midi_metadata_table_t &data);
    bool GetItem(const char *name, midi_metadata_item_t &item) const;
    bool GetBitmap(std::vector<uint8_t> &bitmap) const;
    void AssignBitmap(std::vector<uint8_t>::const_iterator const &begin, std::vector<uint8_t>::const_iterator const &end);
    std::size_t GetCount() const;

    const midi_metadata_item_t &operator[](size_t index) const;

    using midi_metadata_items_t = std::vector<midi_metadata_item_t>;

    using iterator = midi_metadata_items_t::iterator;
    using const_iterator = midi_metadata_items_t::const_iterator;

    iterator begin() {
        return _Items.begin();
    }

    iterator end() {
        return _Items.end();
    }

    const_iterator begin() const {
        return _Items.begin();
    }

    const_iterator end() const {
        return _Items.end();
    }

    const_iterator cbegin() const {
        return _Items.cbegin();
    }

    const_iterator cend() const {
        return _Items.cend();
    }

    const midi_metadata_item_t &front() const noexcept {
        return _Items.front();
    }

    const midi_metadata_item_t &back() const noexcept {
        return _Items.back();
    }

  private:
    std::vector<midi_metadata_item_t> _Items;
    std::vector<uint8_t> _Bitmap;
};

struct midi_item_t {
    uint32_t Time; // in ms
    uint32_t Data;

    midi_item_t() noexcept : Time(0), Data(0) {}

    midi_item_t(uint32_t time, uint32_t data) noexcept : Time(time), Data(data) {}
};

class midi_container_t {
  public:
    midi_container_t() : _Format(), _TimeDivision(), _ExtraPercussionChannel(~0u) {
        _DeviceNames.resize(16);
    }

    void Initialize(uint32_t format, uint32_t division);

    void AddTrack(const midi_track_t &track);
    void AddEventToTrack(size_t trackIndex, const midi_event_t &event);

    // These functions are really only designed to merge and later remove System Exclusive message dumps.
    void MergeTracks(const midi_container_t &source);
    void SetTrackCount(uint32_t count);
    void SetExtraMetaData(const midi_metadata_table_t &data);

    void ApplyHack(uint32_t hack);

    void SerializeAsStream(size_t subSongIndex, std::vector<midi_item_t> &stream, sysex_table_t &sysExTable, uint32_t &loopBegin, uint32_t &loopEnd,
                           uint32_t cleanFlags) const;
    void SerializeAsSMF(std::vector<uint8_t> &data) const;

    void PromoteToType1();

    void TrimStart();

    typedef std::string (*SplitCallback)(uint8_t bank_msb, uint8_t bank_lsb, uint8_t instrument);

    void SplitByInstrumentChanges(SplitCallback callback = nullptr);

    size_t GetSubSongCount() const;
    size_t GetSubSong(size_t index) const;

    uint32_t GetDuration(size_t subsongIndex, bool ms = false) const;

    uint32_t GetFormat() const;
    uint32_t GetTrackCount() const;
    uint32_t GetChannelCount(size_t subSongIndex) const;

    uint32_t GetLoopBeginTimestamp(size_t subSongIndex, bool ms = false) const;
    uint32_t GetLoopEndTimestamp(size_t subSongIndex, bool ms = false) const;

    std::vector<midi_track_t> &GetTracks() {
        return _Tracks;
    }

    void GetMetaData(size_t subSongIndex, midi_metadata_table_t &data);

    void SetExtraPercussionChannel(uint32_t channelNumber) noexcept {
        _ExtraPercussionChannel = channelNumber;
    }

    uint32_t GetExtraPercussionChannel() const noexcept {
        return _ExtraPercussionChannel;
    }

    void DetectLoops(bool detectXMILoops, bool detectMarkerLoops, bool detectRPGMakerLoops, bool detectTouhouLoops, bool detectLeapFrogLoops);

    static void EncodeVariableLengthQuantity(std::vector<uint8_t> &data, uint32_t delta);

  public:
    using miditracks_t = std::vector<midi_track_t>;
    using iterator = miditracks_t::iterator;
    using const_iterator = miditracks_t::const_iterator;

    iterator begin() {
        return _Tracks.begin();
    }

    iterator end() {
        return _Tracks.end();
    }

    const_iterator begin() const {
        return _Tracks.begin();
    }

    const_iterator end() const {
        return _Tracks.end();
    }

    const_iterator cbegin() const {
        return _Tracks.cbegin();
    }

    const_iterator cend() const {
        return _Tracks.cend();
    }

  public:
    enum {
        CleanFlagEMIDI = 1 << 0,
        CleanFlagInstruments = 1 << 1,
        CleanFlagBanks = 1 << 2,
    };

  private:
    void TrimRange(size_t start, size_t end);
    void TrimTempoMap(size_t index, uint32_t base_timestamp);

    uint32_t TimestampToMS(uint32_t timestamp, size_t subsongIndex) const;

    // Normalize port numbers properly
    template <typename T> void LimitPortNumber(T &number) {
        for (size_t i = 0; i < _PortNumbers.size(); ++i) {
            if (_PortNumbers[i] == number) {
                number = (T)i;

                return;
            }
        }

        _PortNumbers.push_back((uint8_t)number);

        number = _PortNumbers.size() - 1;
    }

    template <typename T> void LimitPortNumber(T &number) const {
        for (size_t i = 0; i < _PortNumbers.size(); i++) {
            if (_PortNumbers[i] == number) {
                number = (T)i;

                return;
            }
        }
    }

    void AssignString(const char *src, size_t srcLength, std::string &dst) const {
        dst.assign(src, src + srcLength);
    }

  private:
    uint32_t _Format;
    uint32_t _TimeDivision;
    uint32_t _ExtraPercussionChannel;

    std::vector<uint64_t> _ChannelMask;
    std::vector<tempo_map_t> _TempoMaps;
    std::vector<midi_track_t> _Tracks;

    std::vector<uint8_t> _PortNumbers;

    std::vector<std::vector<std::string>> _DeviceNames;

    midi_metadata_table_t _ExtraMetaData;

    std::vector<uint32_t> _EndTimestamps;

    std::vector<range_t> _Loop;
};
