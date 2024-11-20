//----------------------------------------------------------------------------------------------------------------------
// primesynth: SoundFont-powered MIDI synthesizer (https://github.com/mosmeh/primesynth)
// Copyright (c) 2018 mosm
//
// Modified by a740g for QB64-PE
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace primesynth {
// 64 bit fixed-point number
// higher 32 bit for integer part and lower 32 bit for fractional part
class FixedPoint {
  public:
    FixedPoint() = delete;

    explicit FixedPoint(std::uint32_t integer) : raw_(static_cast<std::uint64_t>(integer) << 32) {}

    explicit FixedPoint(double value)
        : raw_((static_cast<std::uint64_t>(value) << 32) | static_cast<std::uint32_t>((value - static_cast<std::uint32_t>(value)) * (UINT32_MAX + 1.0))) {}

    std::uint32_t getIntegerPart() const {
        return raw_ >> 32;
    }

    double getFractionalPart() const {
        return (raw_ & UINT32_MAX) / (UINT32_MAX + 1.0);
    }

    double getReal() const {
        return getIntegerPart() + getFractionalPart();
    }

    std::uint32_t getRoundedInteger() const {
        return ((raw_ + INT32_MAX) + 1) >> 32;
    }

    FixedPoint &operator+=(const FixedPoint &b) {
        raw_ += b.raw_;
        return *this;
    }

    FixedPoint &operator-=(const FixedPoint &b) {
        raw_ -= b.raw_;
        return *this;
    }

  private:
    std::uint64_t raw_;
};

namespace conv {
void initialize();

// attenuation: centibel
// amplitude:   normalized linear value in [0, 1]
double attenuationToAmplitude(double atten);
double amplitudeToAttenuation(double amp);

double keyToHertz(double key);
double timecentToSecond(double tc);
double absoluteCentToHertz(double ac);

double concave(double x);
double convex(double x);
} // namespace conv

struct StereoValue {
    double left, right;

    StereoValue() = delete;

    StereoValue(double l, double r) : left(l), right(r) {}

    StereoValue operator*(double b) const;
    StereoValue &operator+=(const StereoValue &b);
};

StereoValue operator*(double a, const StereoValue &b);

namespace sf {
enum class SampleLink : std::uint16_t {
    MonoSample = 1,
    RightSample = 2,
    LeftSample = 4,
    LinkedSample = 8,
    RomMonoSample = 0x8001,
    RomRightSample = 0x8002,
    RomLeftSample = 0x8004,
    RomLinkedSample = 0x8008
};

enum class Generator : std::uint16_t {
    StartAddrsOffset = 0,
    EndAddrsOffset = 1,
    StartloopAddrsOffset = 2,
    EndloopAddrsOffset = 3,
    StartAddrsCoarseOffset = 4,
    ModLfoToPitch = 5,
    VibLfoToPitch = 6,
    ModEnvToPitch = 7,
    InitialFilterFc = 8,
    InitialFilterQ = 9,
    ModLfoToFilterFc = 10,
    ModEnvToFilterFc = 11,
    EndAddrsCoarseOffset = 12,
    ModLfoToVolume = 13,
    ChorusEffectsSend = 15,
    ReverbEffectsSend = 16,
    Pan = 17,
    DelayModLFO = 21,
    FreqModLFO = 22,
    DelayVibLFO = 23,
    FreqVibLFO = 24,
    DelayModEnv = 25,
    AttackModEnv = 26,
    HoldModEnv = 27,
    DecayModEnv = 28,
    SustainModEnv = 29,
    ReleaseModEnv = 30,
    KeynumToModEnvHold = 31,
    KeynumToModEnvDecay = 32,
    DelayVolEnv = 33,
    AttackVolEnv = 34,
    HoldVolEnv = 35,
    DecayVolEnv = 36,
    SustainVolEnv = 37,
    ReleaseVolEnv = 38,
    KeynumToVolEnvHold = 39,
    KeynumToVolEnvDecay = 40,
    Instrument = 41,
    KeyRange = 43,
    VelRange = 44,
    StartloopAddrsCoarseOffset = 45,
    Keynum = 46,
    Velocity = 47,
    InitialAttenuation = 48,
    EndloopAddrsCoarseOffset = 50,
    CoarseTune = 51,
    FineTune = 52,
    SampleID = 53,
    SampleModes = 54,
    ScaleTuning = 56,
    ExclusiveClass = 57,
    OverridingRootKey = 58,
    EndOper = 60,
    Pitch, // non-standard generator, used as a destination of default pitch bend modulator
    Last
};

enum class GeneralController : std::uint8_t {
    NoController = 0,
    NoteOnVelocity = 2,
    NoteOnKeyNumber = 3,
    PolyPressure = 10,
    ChannelPressure = 13,
    PitchWheel = 14,
    PitchWheelSensitivity = 16,
    Link = 127
};

enum class ControllerPalette { General = 0, MIDI = 1 };

enum class SourceDirection { Positive = 0, Negative = 1 };

enum class SourcePolarity { Unipolar = 0, Bipolar = 1 };

enum class SourceType { Linear = 0, Concave = 1, Convex = 2, Switch = 3 };

struct Modulator {
    union {
        GeneralController general;
        std::uint8_t midi;
    } index;

    ControllerPalette palette;
    SourceDirection direction;
    SourcePolarity polarity;
    SourceType type;
};

enum class Transform : std::uint16_t { Linear = 0, AbsoluteValue = 2 };

struct RangesType {
    std::int8_t lo;
    std::int8_t hi;
};

union GenAmountType {
    RangesType ranges;
    std::int16_t shAmount;
    std::uint16_t wAmount;
};

struct VersionTag {
    std::uint16_t major;
    std::uint16_t minor;
};

struct PresetHeader {
    char presetName[20];
    std::uint16_t preset;
    std::uint16_t bank;
    std::uint16_t presetBagNdx;
    std::uint32_t library;
    std::uint32_t genre;
    std::uint32_t morphology;
};

struct Bag {
    std::uint16_t genNdx;
    std::uint16_t modNdx;
};

struct ModList {
    Modulator modSrcOper;
    Generator modDestOper;
    std::int16_t modAmount;
    Modulator modAmtSrcOper;
    Transform modTransOper;
};

struct GenList {
    Generator genOper;
    GenAmountType genAmount;
};

struct Inst {
    char instName[20];
    std::uint16_t instBagNdx;
};

struct Sample {
    char sampleName[20];
    std::uint32_t start;
    std::uint32_t end;
    std::uint32_t startloop;
    std::uint32_t endloop;
    std::uint32_t sampleRate;
    std::int8_t originalKey;
    std::int8_t correction;
    std::uint16_t sampleLink;
    SampleLink sampleType;
};
} // namespace sf

static constexpr std::size_t NUM_GENERATORS = static_cast<std::size_t>(sf::Generator::Last);
static constexpr std::uint16_t PERCUSSION_BANK = 128;

struct Sample {
    std::string name;
    std::uint32_t start, end, startLoop, endLoop, sampleRate;
    std::int8_t key, correction;
    double minAtten;
    const std::vector<std::int16_t> &buffer;

    Sample(const sf::Sample &sample, const std::vector<std::int16_t> &sampleBuffer);
};

class GeneratorSet {
  public:
    GeneratorSet();

    std::int16_t getOrDefault(sf::Generator type) const;

    void set(sf::Generator type, std::int16_t amount);
    void merge(const GeneratorSet &b);
    void add(const GeneratorSet &b);

  private:
    struct Generator {
        bool used;
        std::int16_t amount;
    };

    std::array<Generator, NUM_GENERATORS> generators_;
};

class ModulatorParameterSet {
  public:
    static const ModulatorParameterSet &getDefaultParameters();

    const std::vector<sf::ModList> &getParameters() const;

    void append(const sf::ModList &param);
    void addOrAppend(const sf::ModList &param);
    void merge(const ModulatorParameterSet &b);
    void mergeAndAdd(const ModulatorParameterSet &b);

  private:
    std::vector<sf::ModList> params_;
};

struct Zone {
    struct Range {
        std::int8_t min = 0, max = 127;

        bool contains(std::int8_t value) const;
    };

    Range keyRange, velocityRange;
    GeneratorSet generators;
    ModulatorParameterSet modulatorParameters;

    bool isInRange(std::int8_t key, std::int8_t velocity) const;
};

struct Instrument {
    std::string name;
    std::vector<Zone> zones;

    Instrument(std::vector<sf::Inst>::const_iterator instIter, const std::vector<sf::Bag> &ibag, const std::vector<sf::ModList> &imod,
               const std::vector<sf::GenList> &igen);
};

class SoundFont;

struct Preset {
    std::string name;
    std::uint16_t bank, presetID;
    std::vector<Zone> zones;
    const SoundFont &soundFont;

    Preset(std::vector<sf::PresetHeader>::const_iterator phdrIter, const std::vector<sf::Bag> &pbag, const std::vector<sf::ModList> &pmod,
           const std::vector<sf::GenList> &pgen, const SoundFont &sfont);
};

class SoundFont {
  public:
    explicit SoundFont(const std::string &filename);

    const std::string &getName() const;
    const std::vector<Sample> &getSamples() const;
    const std::vector<Instrument> &getInstruments() const;
    const std::vector<std::shared_ptr<const Preset>> &getPresetPtrs() const;

  private:
    std::string name_;
    std::vector<std::int16_t> sampleBuffer_;
    std::vector<Sample> samples_;
    std::vector<Instrument> instruments_;
    std::vector<std::shared_ptr<const Preset>> presets_;

    void readInfoChunk(std::ifstream &ifs, std::size_t size);
    void readSdtaChunk(std::ifstream &ifs, std::size_t size);
    void readPdtaChunk(std::ifstream &ifs, std::size_t size);
};

namespace midi {
static constexpr std::uint8_t PERCUSSION_CHANNEL = 9;
static constexpr std::size_t NUM_CONTROLLERS = 128;
static constexpr std::uint8_t MAX_KEY = 127;

enum class Standard { GM, GS, XG };

enum class MessageStatus {
    NoteOff = 0x80,
    NoteOn = 0x90,
    KeyPressure = 0xa0,
    ControlChange = 0xb0,
    ProgramChange = 0xc0,
    ChannelPressure = 0xd0,
    PitchBend = 0xe0
};

// GM CCs + bank select + RPN/NRPN
enum class ControlChange {
    BankSelectMSB = 0,
    Modulation = 1,
    DataEntryMSB = 6,
    Volume = 7,
    Pan = 10,
    Expression = 11,
    BankSelectLSB = 32,
    DataEntryLSB = 38,
    Sustain = 64,
    DataIncrement = 96,
    DataDecrement = 97,
    NRPNLSB = 98,
    NRPNMSB = 99,
    RPNLSB = 100,
    RPNMSB = 101,
    AllSoundOff = 120,
    ResetAllControllers = 121,
    AllNotesOff = 123
};

// GM RPNs
enum class RPN { PitchBendSensitivity = 0, FineTuning = 1, CoarseTuning = 2, Last };

struct Bank {
    std::uint8_t msb, lsb;
};

std::uint16_t joinBytes(std::uint8_t msb, std::uint8_t lsb);
} // namespace midi

class LFO {
  public:
    LFO(double outputRate, unsigned int interval) : outputRate_(outputRate), interval_(interval), steps_(0), delay_(0), delta_(0.0), value_(0.0), up_(true) {}

    double getValue() const {
        return value_;
    }

    void setDelay(double delay) {
        delay_ = static_cast<unsigned int>(outputRate_ * conv::timecentToSecond(delay));
    }

    void setFrequency(double freq) {
        delta_ = 4.0 * interval_ * conv::absoluteCentToHertz(freq) / outputRate_;
    }

    void update() {
        if (steps_ <= delay_) {
            ++steps_;
            return;
        }
        if (up_) {
            value_ += delta_;
            if (value_ > 1.0) {
                value_ = 2.0 - value_;
                up_ = false;
            }
        } else {
            value_ -= delta_;
            if (value_ < -1.0) {
                value_ = -2.0 - value_;
                up_ = true;
            }
        }
    }

  private:
    const double outputRate_;
    const unsigned int interval_;
    unsigned int steps_, delay_;
    double delta_, value_;
    bool up_;
};

class Envelope {
  public:
    enum class Phase { Delay, Attack, Hold, Decay, Sustain, Release, Finished };

    Envelope(double outputRate, unsigned int interval);

    Phase getPhase() const;
    double getValue() const;

    void setParameter(Phase phase, double param);
    void release();
    void update();

  private:
    const double effectiveOutputRate_;
    std::array<double, static_cast<std::size_t>(Phase::Finished)> params_;
    Phase phase_;
    unsigned int phaseSteps_;
    double value_;

    void changePhase(Phase phase);
};

class Modulator {
  public:
    explicit Modulator(const sf::ModList &param);

    sf::Generator getDestination() const;
    std::int16_t getAmount() const;
    bool canBeNegative() const;
    double getValue() const;

    bool updateSFController(sf::GeneralController controller, double value);
    bool updateMIDIController(std::uint8_t controller, std::uint8_t value);

  private:
    const sf::ModList param_;
    double source_, amountSource_, value_;

    void calculateValue();
};

class Voice {
  public:
    enum class State { Playing, Sustained, Released, Finished };

    Voice(std::size_t noteID, double outputRate, const Sample &sample, const GeneratorSet &generators, const ModulatorParameterSet &modparams, std::uint8_t key,
          std::uint8_t velocity);

    std::size_t getNoteID() const;
    std::uint8_t getActualKey() const;
    std::int16_t getExclusiveClass() const;
    const State &getStatus() const;
    StereoValue render() const;

    void setPercussion(bool percussion);
    void updateSFController(sf::GeneralController controller, double value);
    void updateMIDIController(std::uint8_t controller, std::uint8_t value);
    void updateFineTuning(double fineTuning);
    void updateCoarseTuning(double coarseTuning);
    void release(bool sustained);
    void update();

  private:
    enum class SampleMode { UnLooped, Looped, UnUsed, LoopedUntilRelease };

    struct RuntimeSample {
        SampleMode mode;
        double pitch;
        std::uint32_t start, end, startLoop, endLoop;
    };

    const std::size_t noteID_;
    const std::uint8_t actualKey_;
    const std::vector<std::int16_t> &sampleBuffer_;
    GeneratorSet generators_;
    RuntimeSample rtSample_;
    int keyScaling_;
    std::vector<Modulator> modulators_;
    double minAtten_;
    std::array<double, NUM_GENERATORS> modulated_;
    bool percussion_;
    double fineTuning_, coarseTuning_;
    double deltaIndexRatio_;
    unsigned int steps_;
    State status_;
    double voicePitch_;
    FixedPoint index_, deltaIndex_;
    StereoValue volume_;
    double amp_, deltaAmp_;
    Envelope volEnv_, modEnv_;
    LFO vibLFO_, modLFO_;

    double getModulatedGenerator(sf::Generator type) const;
    void updateModulatedParams(sf::Generator destination);
};

class Channel {
    friend class Synthesizer;

  public:
    explicit Channel(double outputRate);

    midi::Bank getBank() const;
    bool hasPreset() const;

    void noteOff(std::uint8_t key);
    void noteOn(std::uint8_t key, std::uint8_t velocity);
    void keyPressure(std::uint8_t key, std::uint8_t value);
    void controlChange(std::uint8_t controller, std::uint8_t value);
    void channelPressure(std::uint8_t value);
    void pitchBend(std::uint16_t value);
    void setPreset(const std::shared_ptr<const Preset> &preset);
    StereoValue render();

  private:
    enum class DataEntryMode { RPN, NRPN };

    const double outputRate_;
    std::shared_ptr<const Preset> preset_;
    std::array<std::uint8_t, midi::NUM_CONTROLLERS> controllers_;
    std::array<std::uint16_t, static_cast<std::size_t>(midi::RPN::Last)> rpns_;
    std::array<std::uint8_t, midi::MAX_KEY + 1> keyPressures_;
    std::uint8_t channelPressure_;
    std::uint16_t pitchBend_;
    DataEntryMode dataEntryMode_;
    double pitchBendSensitivity_;
    double fineTuning_, coarseTuning_;
    std::vector<std::unique_ptr<Voice>> voices_;
    std::size_t currentNoteID_;

    std::uint16_t getSelectedRPN() const;

    void addVoice(std::unique_ptr<Voice> voice);
    void updateRPN();
};

class Synthesizer {
  public:
    Synthesizer(double outputRate = 44100, std::size_t numChannels = 16);

    void render_float(float *buffer, std::size_t samples);
    void render_s16(std::int16_t *buffer, std::size_t samples);

    void loadSoundFont(const std::string &filename);
    void setVolume(double volume);
    void setMIDIStandard(midi::Standard midiStandard, bool fixed = false);
    void processSysEx(const char *data, std::size_t length);
    void processChannelMessage(midi::MessageStatus event, std::uint8_t chan, std::uint8_t param1 = 0, std::uint8_t param2 = 0);
    void pause(void);
    void stop(void);
    uint32_t getActiveVoiceCount(void) const;

  private:
    midi::Standard midiStd_, defaultMIDIStd_;
    bool stdFixed_;
    std::vector<std::unique_ptr<Channel>> channels_;
    std::vector<std::unique_ptr<SoundFont>> soundFonts_;
    double volume_;

    std::shared_ptr<const Preset> findPreset(std::uint16_t bank, std::uint16_t presetID) const;
};
} // namespace primesynth
