//----------------------------------------------------------------------------------------------------------------------
// primesynth: SoundFont-powered MIDI synthesizer (https://github.com/mosmeh/primesynth)
// Copyright (c) 2018 mosm
//
// Modified by a740g for QB64-PE
//----------------------------------------------------------------------------------------------------------------------

#include "primesynth.h"
#include <climits>
#include <cmath>
#include <fstream>
#include <stdexcept>

namespace primesynth {
namespace conv {
std::array<double, 1441> attenToAmpTable;
std::array<double, 1200> centToHertzTable;

void initialize() {
    static bool initialized = false;
    if (!initialized) {
        initialized = true;

        for (std::size_t i = 0; i < attenToAmpTable.size(); ++i) {
            // -200 instead of -100 for compatibility
            attenToAmpTable.at(i) = std::pow(10.0, i / -200.0);
        }
        for (std::size_t i = 0; i < centToHertzTable.size(); i++) {
            centToHertzTable.at(i) = 6.875 * std::exp2(i / 1200.0);
        }
    }
}

double attenuationToAmplitude(double atten) {
    if (atten <= 0.0) {
        return 1.0;
    } else if (atten >= attenToAmpTable.size()) {
        return 0.0;
    } else {
        return attenToAmpTable.at(static_cast<std::size_t>(atten));
    }
}

double amplitudeToAttenuation(double amp) {
    return -200.0 * std::log10(amp);
}

double keyToHertz(double key) {
    if (key < 0.0) {
        return 1.0;
    }

    int offset = 300;
    int ratio = 1;
    for (int threshold = 900; threshold <= 14100; threshold += 1200) {
        if (key * 100.0 < threshold) {
            return ratio * centToHertzTable.at(static_cast<int>(key * 100.0) + offset);
        }
        offset -= 1200;
        ratio *= 2;
    }

    return 1.0;
}

double timecentToSecond(double tc) {
    return std::exp2(tc / 1200.0);
}

double absoluteCentToHertz(double ac) {
    return 8.176 * std::exp2(ac / 1200.0);
}

double concave(double x) {
    if (x <= 0.0) {
        return 0.0;
    } else if (x >= 1.0) {
        return 1.0;
    } else {
        return 2.0 * conv::amplitudeToAttenuation(1.0 - x) / 960.0;
    }
}

double convex(double x) {
    if (x <= 0.0) {
        return 0.0;
    } else if (x >= 1.0) {
        return 1.0;
    } else {
        return 1.0 - 2.0 * conv::amplitudeToAttenuation(x) / 960.0;
    }
}
} // namespace conv

StereoValue StereoValue::operator*(double b) const {
    return {left * b, right * b};
}

StereoValue &StereoValue::operator+=(const StereoValue &b) {
    left += b.left;
    right += b.right;
    return *this;
}

StereoValue operator*(double a, const StereoValue &b) {
    return {a * b.left, a * b.right};
}

std::string achToString(const char ach[20]) {
    std::string achstr;
    for (std::size_t pos = 0; pos < 20; pos++) {
        if (ach[pos]) {
            achstr.push_back(ach[pos]);
        } else {
            break;
        }
    }
    return achstr;
}

Sample::Sample(const sf::Sample &sample, const std::vector<std::int16_t> &sampleBuffer)
    : name(achToString(sample.sampleName)), start(sample.start), end(sample.end), startLoop(sample.startloop), endLoop(sample.endloop),
      sampleRate(sample.sampleRate), key(sample.originalKey), correction(sample.correction), buffer(sampleBuffer) {
    // if SoundFont file is comformant to specification, generators do not extend sample range beyond start and end
    if (start >= sampleBuffer.size() || end >= sampleBuffer.size()) {
        throw std::runtime_error("Malformed SoundFont! (generator extends sample range beyond end)\n");
    }
    if (start < end) {
        int sampleMax = 0;
        for (std::size_t i = start; i < end; ++i) {
            sampleMax = std::max(sampleMax, std::abs(sampleBuffer.at(i)));
        }
        minAtten = conv::amplitudeToAttenuation(static_cast<double>(sampleMax) / INT16_MAX);
    } else {
        minAtten = INFINITY;
    }
}

static const std::array<std::int16_t, NUM_GENERATORS> DEFAULT_GENERATOR_VALUES = {
    0,      // startAddrsOffset
    0,      // endAddrsOffset
    0,      // startloopAddrsOffset
    0,      // endloopAddrsOffset
    0,      // startAddrsCoarseOffset
    0,      // modLfoToPitch
    0,      // vibLfoToPitch
    0,      // modEnvToPitch
    13500,  // initialFilterFc
    0,      // initialFilterQ
    0,      // modLfoToFilterFc
    0,      // modEnvToFilterFc
    0,      // endAddrsCoarseOffset
    0,      // modLfoToVolume
    0,      // unused1
    0,      // chorusEffectsSend
    0,      // reverbEffectsSend
    0,      // pan
    0,      // unused2
    0,      // unused3
    0,      // unused4
    -12000, // delayModLFO
    0,      // freqModLFO
    -12000, // delayVibLFO
    0,      // freqVibLFO
    -12000, // delayModEnv
    -12000, // attackModEnv
    -12000, // holdModEnv
    -12000, // decayModEnv
    0,      // sustainModEnv
    -12000, // releaseModEnv
    0,      // keynumToModEnvHold
    0,      // keynumToModEnvDecay
    -12000, // delayVolEnv
    -12000, // attackVolEnv
    -12000, // holdVolEnv
    -12000, // decayVolEnv
    0,      // sustainVolEnv
    -12000, // releaseVolEnv
    0,      // keynumToVolEnvHold
    0,      // keynumToVolEnvDecay
    0,      // instrument
    0,      // reserved1
    0,      // keyRange
    0,      // velRange
    0,      // startloopAddrsCoarseOffset
    -1,     // keynum
    -1,     // velocity
    0,      // initialAttenuation
    0,      // reserved2
    0,      // endloopAddrsCoarseOffset
    0,      // coarseTune
    0,      // fineTune
    0,      // sampleID
    0,      // sampleModes
    0,      // reserved3
    100,    // scaleTuning
    0,      // exclusiveClass
    -1,     // overridingRootKey
    0,      // unused5
    0,      // endOper
    0       // pitch
};

GeneratorSet::GeneratorSet() {
    for (std::size_t i = 0; i < NUM_GENERATORS; ++i) {
        generators_.at(i) = {false, DEFAULT_GENERATOR_VALUES.at(i)};
    }
}

std::int16_t GeneratorSet::getOrDefault(sf::Generator type) const {
    return generators_.at(static_cast<std::size_t>(type)).amount;
}

void GeneratorSet::set(sf::Generator type, std::int16_t amount) {
    generators_.at(static_cast<std::size_t>(type)) = {true, amount};
}

void GeneratorSet::merge(const GeneratorSet &b) {
    for (std::size_t i = 0; i < NUM_GENERATORS; ++i) {
        if (!generators_.at(i).used && b.generators_.at(i).used) {
            generators_.at(i) = b.generators_.at(i);
        }
    }
}

void GeneratorSet::add(const GeneratorSet &b) {
    for (std::size_t i = 0; i < NUM_GENERATORS; ++i) {
        if (b.generators_.at(i).used) {
            generators_.at(i).amount += b.generators_.at(i).amount;
            generators_.at(i).used = true;
        }
    }
}

const ModulatorParameterSet &ModulatorParameterSet::getDefaultParameters() {
    static ModulatorParameterSet params;
    static bool initialized = false;
    if (!initialized) {
        initialized = true;

        // See "SoundFont Technical Specification" Version 2.04
        // p.41 "8.4 Default Modulators"
        {
            // 8.4.1 MIDI Note-On Velocity to Initial Attenuation
            sf::ModList param;
            param.modSrcOper.index.general = sf::GeneralController::NoteOnVelocity;
            param.modSrcOper.palette = sf::ControllerPalette::General;
            param.modSrcOper.direction = sf::SourceDirection::Negative;
            param.modSrcOper.polarity = sf::SourcePolarity::Unipolar;
            param.modSrcOper.type = sf::SourceType::Concave;
            param.modDestOper = sf::Generator::InitialAttenuation;
            param.modAmount = 960;
            param.modAmtSrcOper.index.general = sf::GeneralController::NoController;
            param.modAmtSrcOper.palette = sf::ControllerPalette::General;
            param.modTransOper = sf::Transform::Linear;
            params.append(param);
        }
        {
            // 8.4.2 MIDI Note-On Velocity to Filter Cutoff
            sf::ModList param;
            param.modSrcOper.index.general = sf::GeneralController::NoteOnVelocity;
            param.modSrcOper.palette = sf::ControllerPalette::General;
            param.modSrcOper.direction = sf::SourceDirection::Negative;
            param.modSrcOper.polarity = sf::SourcePolarity::Unipolar;
            param.modSrcOper.type = sf::SourceType::Linear;
            param.modDestOper = sf::Generator::InitialFilterFc;
            param.modAmount = -2400;
            param.modAmtSrcOper.index.general = sf::GeneralController::NoController;
            param.modAmtSrcOper.palette = sf::ControllerPalette::General;
            param.modTransOper = sf::Transform::Linear;
            params.append(param);
        }
        {
            // 8.4.3 MIDI Channel Pressure to Vibrato LFO Pitch Depth
            sf::ModList param;
            param.modSrcOper.index.midi = 13;
            param.modSrcOper.palette = sf::ControllerPalette::MIDI;
            param.modSrcOper.direction = sf::SourceDirection::Positive;
            param.modSrcOper.polarity = sf::SourcePolarity::Unipolar;
            param.modSrcOper.type = sf::SourceType::Linear;
            param.modDestOper = sf::Generator::VibLfoToPitch;
            param.modAmount = 50;
            param.modAmtSrcOper.index.general = sf::GeneralController::NoController;
            param.modAmtSrcOper.palette = sf::ControllerPalette::General;
            param.modTransOper = sf::Transform::Linear;
            params.append(param);
        }
        {
            // 8.4.4 MIDI Continuous Controller 1 to Vibrato LFO Pitch Depth
            sf::ModList param;
            param.modSrcOper.index.midi = 1;
            param.modSrcOper.palette = sf::ControllerPalette::MIDI;
            param.modSrcOper.direction = sf::SourceDirection::Positive;
            param.modSrcOper.polarity = sf::SourcePolarity::Unipolar;
            param.modSrcOper.type = sf::SourceType::Linear;
            param.modDestOper = sf::Generator::VibLfoToPitch;
            param.modAmount = 50;
            param.modAmtSrcOper.index.general = sf::GeneralController::NoController;
            param.modAmtSrcOper.palette = sf::ControllerPalette::General;
            param.modTransOper = sf::Transform::Linear;
            params.append(param);
        }
        {
            // 8.4.5 MIDI Continuous Controller 7 to Initial Attenuation Source
            sf::ModList param;
            param.modSrcOper.index.midi = 7;
            param.modSrcOper.palette = sf::ControllerPalette::MIDI;
            param.modSrcOper.direction = sf::SourceDirection::Negative;
            param.modSrcOper.polarity = sf::SourcePolarity::Unipolar;
            param.modSrcOper.type = sf::SourceType::Concave;
            param.modDestOper = sf::Generator::InitialAttenuation;
            param.modAmount = 960;
            param.modAmtSrcOper.index.general = sf::GeneralController::NoController;
            param.modAmtSrcOper.palette = sf::ControllerPalette::General;
            param.modTransOper = sf::Transform::Linear;
            params.append(param);
        }
        {
            // 8.4.6 MIDI Continuous Controller 10 to Pan Position
            sf::ModList param;
            param.modSrcOper.index.midi = 10;
            param.modSrcOper.palette = sf::ControllerPalette::MIDI;
            param.modSrcOper.direction = sf::SourceDirection::Positive;
            param.modSrcOper.polarity = sf::SourcePolarity::Bipolar;
            param.modSrcOper.type = sf::SourceType::Linear;
            param.modDestOper = sf::Generator::Pan;
            param.modAmount = 500;
            param.modAmtSrcOper.index.general = sf::GeneralController::NoController;
            param.modAmtSrcOper.palette = sf::ControllerPalette::General;
            param.modTransOper = sf::Transform::Linear;
            params.append(param);
        }
        {
            // 8.4.7 MIDI Continuous Controller 11 to Initial Attenuation
            sf::ModList param;
            param.modSrcOper.index.midi = 11;
            param.modSrcOper.palette = sf::ControllerPalette::MIDI;
            param.modSrcOper.direction = sf::SourceDirection::Negative;
            param.modSrcOper.polarity = sf::SourcePolarity::Unipolar;
            param.modSrcOper.type = sf::SourceType::Concave;
            param.modDestOper = sf::Generator::InitialAttenuation;
            param.modAmount = 960;
            param.modAmtSrcOper.index.general = sf::GeneralController::NoController;
            param.modAmtSrcOper.palette = sf::ControllerPalette::General;
            param.modTransOper = sf::Transform::Linear;
            params.append(param);
        }
        {
            // 8.4.8 MIDI Continuous Controller 91 to Reverb Effects Send
            sf::ModList param;
            param.modSrcOper.index.midi = 91;
            param.modSrcOper.palette = sf::ControllerPalette::MIDI;
            param.modSrcOper.direction = sf::SourceDirection::Positive;
            param.modSrcOper.polarity = sf::SourcePolarity::Unipolar;
            param.modSrcOper.type = sf::SourceType::Linear;
            param.modDestOper = sf::Generator::ReverbEffectsSend;
            param.modAmount = 200;
            param.modAmtSrcOper.index.general = sf::GeneralController::NoController;
            param.modAmtSrcOper.palette = sf::ControllerPalette::General;
            param.modTransOper = sf::Transform::Linear;
            params.append(param);
        }
        {
            // 8.4.9 MIDI Continuous Controller 93 to Chorus Effects Send
            sf::ModList param;
            param.modSrcOper.index.midi = 93;
            param.modSrcOper.palette = sf::ControllerPalette::MIDI;
            param.modSrcOper.direction = sf::SourceDirection::Positive;
            param.modSrcOper.polarity = sf::SourcePolarity::Unipolar;
            param.modSrcOper.type = sf::SourceType::Linear;
            param.modDestOper = sf::Generator::ChorusEffectsSend;
            param.modAmount = 200;
            param.modAmtSrcOper.index.general = sf::GeneralController::NoController;
            param.modAmtSrcOper.palette = sf::ControllerPalette::General;
            param.modTransOper = sf::Transform::Linear;
            params.append(param);
        }
        {
            // 8.4.10 MIDI Pitch Wheel to Initial Pitch Controlled by MIDI Pitch Wheel Sensitivity
            sf::ModList param;
            param.modSrcOper.index.general = sf::GeneralController::PitchWheel;
            param.modSrcOper.palette = sf::ControllerPalette::General;
            param.modSrcOper.direction = sf::SourceDirection::Positive;
            param.modSrcOper.polarity = sf::SourcePolarity::Bipolar;
            param.modSrcOper.type = sf::SourceType::Linear;
            param.modDestOper = sf::Generator::Pitch;
            param.modAmount = 12700;
            param.modAmtSrcOper.index.general = sf::GeneralController::PitchWheelSensitivity;
            param.modAmtSrcOper.palette = sf::ControllerPalette::General;
            param.modAmtSrcOper.direction = sf::SourceDirection::Positive;
            param.modAmtSrcOper.polarity = sf::SourcePolarity::Unipolar;
            param.modAmtSrcOper.type = sf::SourceType::Linear;
            param.modTransOper = sf::Transform::Linear;
            params.append(param);
        }
    }
    return params;
}

const std::vector<sf::ModList> &ModulatorParameterSet::getParameters() const {
    return params_;
}

bool operator==(const sf::Modulator &a, const sf::Modulator &b) {
    return a.index.midi == b.index.midi && a.palette == b.palette && a.direction == b.direction && a.polarity == b.polarity && a.type == b.type;
}

bool modulatorsAreIdentical(const sf::ModList &a, const sf::ModList &b) {
    return a.modSrcOper == b.modSrcOper && a.modDestOper == b.modDestOper && a.modAmtSrcOper == b.modAmtSrcOper && a.modTransOper == b.modTransOper;
}

void ModulatorParameterSet::append(const sf::ModList &param) {
    for (const auto &p : params_) {
        if (modulatorsAreIdentical(p, param)) {
            return;
        }
    }
    params_.push_back(param);
}

void ModulatorParameterSet::addOrAppend(const sf::ModList &param) {
    for (auto &p : params_) {
        if (modulatorsAreIdentical(p, param)) {
            p.modAmount += param.modAmount;
            return;
        }
    }
    params_.push_back(param);
}

void ModulatorParameterSet::merge(const ModulatorParameterSet &b) {
    for (const auto &param : b.params_) {
        append(param);
    }
}

void ModulatorParameterSet::mergeAndAdd(const ModulatorParameterSet &b) {
    for (const auto &param : b.params_) {
        addOrAppend(param);
    }
}

bool Zone::Range::contains(std::int8_t value) const {
    return min <= value && value <= max;
}

bool Zone::isInRange(std::int8_t key, std::int8_t velocity) const {
    return keyRange.contains(key) && velocityRange.contains(velocity);
}

void readBags(std::vector<Zone> &zones, std::vector<sf::Bag>::const_iterator bagBegin, std::vector<sf::Bag>::const_iterator bagEnd,
              const std::vector<sf::ModList> &mods, const std::vector<sf::GenList> &gens, sf::Generator indexGen) {
    if (bagBegin > bagEnd) {
        throw std::runtime_error("bag indices not monotonically increasing");
    }

    Zone globalZone;

    for (auto it_bag = bagBegin; it_bag != bagEnd; ++it_bag) {
        Zone zone;

        const auto &beginMod = mods.begin() + it_bag->modNdx;
        const auto &endMod = mods.begin() + std::next(it_bag)->modNdx;
        if (beginMod > endMod) {
            throw std::runtime_error("modulator indices not monotonically increasing");
        }
        for (auto it_mod = beginMod; it_mod != endMod; ++it_mod) {
            zone.modulatorParameters.append(*it_mod);
        }

        const auto &beginGen = gens.begin() + it_bag->genNdx;
        const auto &endGen = gens.begin() + std::next(it_bag)->genNdx;
        if (beginGen > endGen) {
            throw std::runtime_error("generator indices not monotonically increasing");
        }
        for (auto it_gen = beginGen; it_gen != endGen; ++it_gen) {
            const auto &range = it_gen->genAmount.ranges;
            switch (it_gen->genOper) {
            case sf::Generator::KeyRange:
                zone.keyRange = {range.lo, range.hi};
                break;
            case sf::Generator::VelRange:
                zone.velocityRange = {range.lo, range.hi};
                break;
            default:
                if (it_gen->genOper < sf::Generator::EndOper) {
                    zone.generators.set(it_gen->genOper, it_gen->genAmount.shAmount);
                }
                break;
            }
        }

        if (beginGen != endGen && std::prev(endGen)->genOper == indexGen) {
            zones.push_back(zone);
        } else if (it_bag == bagBegin && (beginGen != endGen || beginMod != endMod)) {
            globalZone = zone;
        }
    }

    for (auto &zone : zones) {
        zone.generators.merge(globalZone.generators);
        zone.modulatorParameters.merge(globalZone.modulatorParameters);
    }
}

Instrument::Instrument(std::vector<sf::Inst>::const_iterator instIter, const std::vector<sf::Bag> &ibag, const std::vector<sf::ModList> &imod,
                       const std::vector<sf::GenList> &igen)
    : name(achToString(instIter->instName)) {
    readBags(zones, ibag.begin() + instIter->instBagNdx, ibag.begin() + std::next(instIter)->instBagNdx, imod, igen, sf::Generator::SampleID);
}

Preset::Preset(std::vector<sf::PresetHeader>::const_iterator phdrIter, const std::vector<sf::Bag> &pbag, const std::vector<sf::ModList> &pmod,
               const std::vector<sf::GenList> &pgen, const SoundFont &sfont)
    : name(achToString(phdrIter->presetName)), bank(phdrIter->bank), presetID(phdrIter->preset), soundFont(sfont) {
    readBags(zones, pbag.begin() + phdrIter->presetBagNdx, pbag.begin() + std::next(phdrIter)->presetBagNdx, pmod, pgen, sf::Generator::Instrument);
}

struct RIFFHeader {
    std::uint32_t id;
    std::uint32_t size;
};

RIFFHeader readHeader(std::ifstream &ifs) {
    RIFFHeader header;
    ifs.read(reinterpret_cast<char *>(&header), sizeof(header));
    return header;
}

std::uint32_t readFourCC(std::ifstream &ifs) {
    std::uint32_t id;
    ifs.read(reinterpret_cast<char *>(&id), sizeof(id));
    return id;
}

constexpr std::uint32_t toFourCC(const char str[5]) {
    std::uint32_t fourCC = 0;
    for (int i = 0; i < 4; ++i) {
        fourCC |= str[i] << CHAR_BIT * i;
    }
    return fourCC;
}

SoundFont::SoundFont(const std::string &filename) {
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs) {
        throw std::runtime_error("failed to open file");
    }

    const RIFFHeader riffHeader = readHeader(ifs);
    const std::uint32_t riffType = readFourCC(ifs);
    if (riffHeader.id != toFourCC("RIFF") || riffType != toFourCC("sfbk")) {
        throw std::runtime_error("not a SoundFont file");
    }

    for (std::size_t s = 0; s < riffHeader.size - sizeof(riffType);) {
        const RIFFHeader chunkHeader = readHeader(ifs);
        s += sizeof(chunkHeader) + chunkHeader.size;
        switch (chunkHeader.id) {
        case toFourCC("LIST"): {
            const std::uint32_t chunkType = readFourCC(ifs);
            const std::size_t chunkSize = chunkHeader.size - sizeof(chunkType);
            switch (chunkType) {
            case toFourCC("INFO"):
                readInfoChunk(ifs, chunkSize);
                break;
            case toFourCC("sdta"):
                readSdtaChunk(ifs, chunkSize);
                break;
            case toFourCC("pdta"):
                readPdtaChunk(ifs, chunkSize);
                break;
            default:
                ifs.ignore(chunkSize);
                break;
            }
            break;
        }
        default:
            ifs.ignore(chunkHeader.size);
            break;
        }
    }
}

const std::string &SoundFont::getName() const {
    return name_;
}

const std::vector<Sample> &SoundFont::getSamples() const {
    return samples_;
}

const std::vector<Instrument> &SoundFont::getInstruments() const {
    return instruments_;
}

const std::vector<std::shared_ptr<const Preset>> &SoundFont::getPresetPtrs() const {
    return presets_;
}

void SoundFont::readInfoChunk(std::ifstream &ifs, std::size_t size) {
    for (std::size_t s = 0; s < size;) {
        const RIFFHeader subchunkHeader = readHeader(ifs);
        s += sizeof(subchunkHeader) + subchunkHeader.size;
        switch (subchunkHeader.id) {
        case toFourCC("ifil"): {
            sf::VersionTag ver;
            ifs.read(reinterpret_cast<char *>(&ver), subchunkHeader.size);
            if (ver.major > 2 || ver.minor > 4) {
                throw std::runtime_error("SoundFont later than 2.04 not supported");
            }
            break;
        }
        case toFourCC("INAM"): {
            std::vector<char> buf(subchunkHeader.size);
            ifs.read(buf.data(), buf.size());
            name_ = buf.data();
            break;
        }
        default:
            ifs.ignore(subchunkHeader.size);
            break;
        }
    }
}

void SoundFont::readSdtaChunk(std::ifstream &ifs, std::size_t size) {
    for (std::size_t s = 0; s < size;) {
        const RIFFHeader subchunkHeader = readHeader(ifs);
        s += sizeof(subchunkHeader) + subchunkHeader.size;
        switch (subchunkHeader.id) {
        case toFourCC("smpl"):
            if (subchunkHeader.size == 0) {
                throw std::runtime_error("no sample data found");
            }
            sampleBuffer_.resize(subchunkHeader.size / sizeof(std::int16_t));
            ifs.read(reinterpret_cast<char *>(sampleBuffer_.data()), subchunkHeader.size);
            break;
        default:
            ifs.ignore(subchunkHeader.size);
            break;
        }
    }
}

template <typename T> void readPdtaList(std::ifstream &ifs, std::vector<T> &list, std::uint32_t totalSize, std::size_t structSize) {
    if (totalSize % structSize != 0) {
        throw std::runtime_error("invalid chunk size");
    }
    list.resize(totalSize / structSize);
    for (std::size_t i = 0; i < totalSize / structSize; ++i) {
        ifs.read(reinterpret_cast<char *>(&list.at(i)), structSize);
    }
}

void readModulator(std::ifstream &ifs, sf::Modulator &mod) {
    std::uint16_t data;
    ifs.read(reinterpret_cast<char *>(&data), 2);

    mod.index.midi = data & 127;
    mod.palette = static_cast<sf::ControllerPalette>((data >> 7) & 1);
    mod.direction = static_cast<sf::SourceDirection>((data >> 8) & 1);
    mod.polarity = static_cast<sf::SourcePolarity>((data >> 9) & 1);
    mod.type = static_cast<sf::SourceType>((data >> 10) & 63);
}

void readModList(std::ifstream &ifs, std::vector<sf::ModList> &list, std::uint32_t totalSize) {
    static const size_t STRUCT_SIZE = 10;
    if (totalSize % STRUCT_SIZE != 0) {
        throw std::runtime_error("invalid chunk size");
    }
    list.reserve(totalSize / STRUCT_SIZE);
    for (std::size_t i = 0; i < totalSize / STRUCT_SIZE; ++i) {
        sf::ModList mod;
        readModulator(ifs, mod.modSrcOper);
        ifs.read(reinterpret_cast<char *>(&mod.modDestOper), 2);
        ifs.read(reinterpret_cast<char *>(&mod.modAmount), 2);
        readModulator(ifs, mod.modAmtSrcOper);
        ifs.read(reinterpret_cast<char *>(&mod.modTransOper), 2);
        list.push_back(mod);
    }
}

void SoundFont::readPdtaChunk(std::ifstream &ifs, std::size_t size) {
    std::vector<sf::PresetHeader> phdr;
    std::vector<sf::Inst> inst;
    std::vector<sf::Bag> pbag, ibag;
    std::vector<sf::ModList> pmod, imod;
    std::vector<sf::GenList> pgen, igen;
    std::vector<sf::Sample> shdr;

    for (std::size_t s = 0; s < size;) {
        const RIFFHeader subchunkHeader = readHeader(ifs);
        s += sizeof(subchunkHeader) + subchunkHeader.size;
        switch (subchunkHeader.id) {
        case toFourCC("phdr"):
            readPdtaList(ifs, phdr, subchunkHeader.size, 38);
            break;
        case toFourCC("pbag"):
            readPdtaList(ifs, pbag, subchunkHeader.size, 4);
            break;
        case toFourCC("pmod"):
            readModList(ifs, pmod, subchunkHeader.size);
            break;
        case toFourCC("pgen"):
            readPdtaList(ifs, pgen, subchunkHeader.size, 4);
            break;
        case toFourCC("inst"):
            readPdtaList(ifs, inst, subchunkHeader.size, 22);
            break;
        case toFourCC("ibag"):
            readPdtaList(ifs, ibag, subchunkHeader.size, 4);
            break;
        case toFourCC("imod"):
            readModList(ifs, imod, subchunkHeader.size);
            break;
        case toFourCC("igen"):
            readPdtaList(ifs, igen, subchunkHeader.size, 4);
            break;
        case toFourCC("shdr"):
            readPdtaList(ifs, shdr, subchunkHeader.size, 46);
            break;
        default:
            ifs.ignore(subchunkHeader.size);
            break;
        }
    }

    // last records of inst, phdr, and shdr sub-chunks indicate end of records, and are ignored

    if (inst.size() < 2) {
        throw std::runtime_error("no instrument found");
    }
    instruments_.reserve(inst.size() - 1);
    for (auto it_inst = inst.begin(); it_inst != std::prev(inst.end()); ++it_inst) {
        instruments_.emplace_back(it_inst, ibag, imod, igen);
    }

    if (phdr.size() < 2) {
        throw std::runtime_error("no preset found");
    }
    presets_.reserve(phdr.size() - 1);
    for (auto it_phdr = phdr.begin(); it_phdr != std::prev(phdr.end()); ++it_phdr) {
        presets_.emplace_back(std::make_shared<Preset>(it_phdr, pbag, pmod, pgen, *this));
    }

    if (shdr.size() < 2) {
        throw std::runtime_error("no sample found");
    }
    samples_.reserve(shdr.size() - 1);
    for (auto it_shdr = shdr.begin(); it_shdr != std::prev(shdr.end()); ++it_shdr) {
        samples_.emplace_back(*it_shdr, sampleBuffer_);
    }
}

namespace midi {
std::uint16_t joinBytes(std::uint8_t msb, std::uint8_t lsb) {
    return (static_cast<std::uint16_t>(msb) << 7) + static_cast<std::uint16_t>(lsb);
}
} // namespace midi

Envelope::Envelope(double outputRate, unsigned int interval)
    : effectiveOutputRate_(outputRate / interval), params_(), phase_(Phase::Delay), phaseSteps_(0), value_(1.0) {}

Envelope::Phase Envelope::getPhase() const {
    return phase_;
}

double Envelope::getValue() const {
    return value_;
}

void Envelope::setParameter(Phase phase, double param) {
    if (phase == Phase::Sustain) {
        params_.at(static_cast<std::size_t>(Phase::Sustain)) = 1.0 - 0.001 * param;
    } else if (phase < Phase::Finished) {
        params_.at(static_cast<std::size_t>(phase)) = effectiveOutputRate_ * conv::timecentToSecond(param);
    } else {
        throw std::invalid_argument("unknown phase");
    }
}

void Envelope::release() {
    if (phase_ < Phase::Release) {
        changePhase(Phase::Release);
    }
}

void Envelope::update() {
    if (phase_ == Phase::Finished) {
        return;
    }

    ++phaseSteps_;

    auto i = static_cast<std::size_t>(phase_);
    while (phase_ < Phase::Finished && phase_ != Phase::Sustain && phaseSteps_ >= params_.at(i)) {
        changePhase(static_cast<Phase>(++i));
    }

    const double &sustain = params_.at(static_cast<std::size_t>(Phase::Sustain));
    switch (phase_) {
    case Phase::Delay:
    case Phase::Finished:
        value_ = 0.0;
        return;
    case Phase::Attack:
        value_ = phaseSteps_ / params_.at(i);
        return;
    case Phase::Hold:
        value_ = 1.0;
        return;
    case Phase::Decay:
        value_ = 1.0 - phaseSteps_ / params_.at(i);
        if (value_ <= sustain) {
            value_ = sustain;
            changePhase(Phase::Sustain);
        }
        return;
    case Phase::Sustain:
        value_ = sustain;
        return;
    case Phase::Release:
        value_ -= 1.0 / params_.at(i);
        if (value_ <= 0.0) {
            value_ = 0.0;
            changePhase(Phase::Finished);
        }
        return;
    }

    throw std::logic_error("unreachable");
}

void Envelope::changePhase(Phase phase) {
    phase_ = phase;
    phaseSteps_ = 0;
}

Modulator::Modulator(const sf::ModList &param) : param_(param), source_(0.0), amountSource_(1.0), value_(0.0) {}

sf::Generator Modulator::getDestination() const {
    return param_.modDestOper;
}

std::int16_t Modulator::getAmount() const {
    return param_.modAmount;
}

bool Modulator::canBeNegative() const {
    if (param_.modTransOper == sf::Transform::AbsoluteValue || param_.modAmount == 0) {
        return false;
    }

    if (param_.modAmount > 0) {
        const bool noSrc =
            param_.modSrcOper.palette == sf::ControllerPalette::General && param_.modSrcOper.index.general == sf::GeneralController::NoController;
        const bool uniSrc = param_.modSrcOper.polarity == sf::SourcePolarity::Unipolar;
        const bool noAmt =
            param_.modAmtSrcOper.palette == sf::ControllerPalette::General && param_.modAmtSrcOper.index.general == sf::GeneralController::NoController;
        const bool uniAmt = param_.modAmtSrcOper.polarity == sf::SourcePolarity::Unipolar;

        if ((uniSrc && uniAmt) || (uniSrc && noAmt) || (noSrc && uniAmt) || (noSrc && noAmt)) {
            return false;
        }
    }

    return true;
}

double Modulator::getValue() const {
    return value_;
}

double map(double value, const sf::Modulator &mod) {
    if (mod.palette == sf::ControllerPalette::General && mod.index.general == sf::GeneralController::PitchWheel) {
        value /= 1 << 14;
    } else {
        value /= 1 << 7;
    }

    if (mod.type == sf::SourceType::Switch) {
        const double off = mod.polarity == sf::SourcePolarity::Unipolar ? 0.0 : -1.0;
        const double x = mod.direction == sf::SourceDirection::Positive ? value : 1.0 - value;
        return x >= 0.5 ? 1.0 : off;
    } else if (mod.polarity == sf::SourcePolarity::Unipolar) {
        const double x = mod.direction == sf::SourceDirection::Positive ? value : 1.0 - value;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
        switch (mod.type) {
        case sf::SourceType::Linear:
            return x;
        case sf::SourceType::Concave:
            return conv::concave(x);
        case sf::SourceType::Convex:
            return conv::convex(x);
        }
#pragma GCC diagnostic pop
    } else {
        const int dir = mod.direction == sf::SourceDirection::Positive ? 1 : -1;
        const int sign = value > 0.5 ? 1 : -1;
        const double x = 2.0 * value - 1.0;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
        switch (mod.type) {
        case sf::SourceType::Linear:
            return dir * x;
        case sf::SourceType::Concave:
            return sign * dir * conv::concave(sign * x);
        case sf::SourceType::Convex:
            return sign * dir * conv::convex(sign * x);
        }
#pragma GCC diagnostic pop
    }
    throw std::runtime_error("unknown modulator controller type");
}

bool Modulator::updateSFController(sf::GeneralController controller, double value) {
    bool updated = false;
    if (param_.modSrcOper.palette == sf::ControllerPalette::General && controller == param_.modSrcOper.index.general) {
        source_ = map(value, param_.modSrcOper);
        updated = true;
    }
    if (param_.modAmtSrcOper.palette == sf::ControllerPalette::General && controller == param_.modAmtSrcOper.index.general) {
        amountSource_ = map(value, param_.modAmtSrcOper);
        updated = true;
    }

    if (updated) {
        calculateValue();
    }
    return updated;
}

bool Modulator::updateMIDIController(std::uint8_t controller, std::uint8_t value) {
    bool updated = false;
    if (param_.modSrcOper.palette == sf::ControllerPalette::MIDI && controller == param_.modSrcOper.index.midi) {
        source_ = map(value, param_.modSrcOper);
        updated = true;
    }
    if (param_.modAmtSrcOper.palette == sf::ControllerPalette::MIDI && controller == param_.modAmtSrcOper.index.midi) {
        amountSource_ = map(value, param_.modAmtSrcOper);
        updated = true;
    }

    if (updated) {
        calculateValue();
    }
    return updated;
}

double transform(double value, sf::Transform transform) {
    switch (transform) {
    case sf::Transform::Linear:
        return value;
    case sf::Transform::AbsoluteValue:
        return std::abs(value);
    }
    throw std::invalid_argument("unknown transform");
}

void Modulator::calculateValue() {
    value_ = transform(param_.modAmount * source_ * amountSource_, param_.modTransOper);
}

static constexpr unsigned int CALC_INTERVAL = 64;

// for compatibility
static constexpr double ATTEN_FACTOR = 0.4;

Voice::Voice(std::size_t noteID, double outputRate, const Sample &sample, const GeneratorSet &generators, const ModulatorParameterSet &modparams,
             std::uint8_t key, std::uint8_t velocity)
    : noteID_(noteID), actualKey_(key), sampleBuffer_(sample.buffer), generators_(generators), percussion_(false), fineTuning_(0.0), coarseTuning_(0.0),
      steps_(0), status_(State::Playing), index_(sample.start), deltaIndex_(0u), volume_({1.0, 1.0}), amp_(0.0), deltaAmp_(0.0),
      volEnv_(outputRate, CALC_INTERVAL), modEnv_(outputRate, CALC_INTERVAL), vibLFO_(outputRate, CALC_INTERVAL), modLFO_(outputRate, CALC_INTERVAL) {
    rtSample_.mode = static_cast<SampleMode>(0b11 & generators.getOrDefault(sf::Generator::SampleModes));
    const std::int16_t overriddenSampleKey = generators.getOrDefault(sf::Generator::OverridingRootKey);
    rtSample_.pitch = (overriddenSampleKey > 0 ? overriddenSampleKey : sample.key) - 0.01 * sample.correction;

    static constexpr std::uint32_t COARSE_UNIT = 32768;
    rtSample_.start =
        sample.start + COARSE_UNIT * generators.getOrDefault(sf::Generator::StartAddrsCoarseOffset) + generators.getOrDefault(sf::Generator::StartAddrsOffset);
    rtSample_.end =
        sample.end + COARSE_UNIT * generators.getOrDefault(sf::Generator::EndAddrsCoarseOffset) + generators.getOrDefault(sf::Generator::EndAddrsOffset);
    rtSample_.startLoop = sample.startLoop + COARSE_UNIT * generators.getOrDefault(sf::Generator::StartloopAddrsCoarseOffset) +
                          generators.getOrDefault(sf::Generator::StartloopAddrsOffset);
    rtSample_.endLoop = sample.endLoop + COARSE_UNIT * generators.getOrDefault(sf::Generator::EndloopAddrsCoarseOffset) +
                        generators.getOrDefault(sf::Generator::EndloopAddrsOffset);

    // fix invalid sample range
    const auto bufferSize = static_cast<std::uint32_t>(sample.buffer.size());
    rtSample_.start = std::min(bufferSize - 1, rtSample_.start);
    rtSample_.end = std::max(rtSample_.start + 1, std::min(bufferSize, rtSample_.end));
    rtSample_.startLoop = std::max(rtSample_.start, std::min(rtSample_.end - 1, rtSample_.startLoop));
    rtSample_.endLoop = std::max(rtSample_.startLoop + 1, std::min(rtSample_.end, rtSample_.endLoop));

    deltaIndexRatio_ = 1.0 / conv::keyToHertz(rtSample_.pitch) * sample.sampleRate / outputRate;

    for (const auto &mp : modparams.getParameters()) {
        modulators_.emplace_back(mp);
    }

    const std::int16_t genVelocity = generators.getOrDefault(sf::Generator::Velocity);
    updateSFController(sf::GeneralController::NoteOnVelocity, genVelocity > 0 ? genVelocity : velocity);

    const std::int16_t genKey = generators.getOrDefault(sf::Generator::Keynum);
    const std::int16_t overriddenKey = genKey > 0 ? genKey : key;
    keyScaling_ = 60 - overriddenKey;
    updateSFController(sf::GeneralController::NoteOnKeyNumber, overriddenKey);

    double minModulatedAtten = ATTEN_FACTOR * generators_.getOrDefault(sf::Generator::InitialAttenuation);
    for (const auto &mod : modulators_) {
        if (mod.getDestination() == sf::Generator::InitialAttenuation && mod.canBeNegative()) {
            // mod may increase volume
            minModulatedAtten -= std::abs(mod.getAmount());
        }
    }
    minAtten_ = sample.minAtten + std::max(0.0, minModulatedAtten);

    for (int i = 0; i < NUM_GENERATORS; ++i) {
        modulated_.at(i) = generators.getOrDefault(static_cast<sf::Generator>(i));
    }

    static const auto INIT_GENERATORS = {sf::Generator::Pan,           sf::Generator::DelayModLFO,   sf::Generator::FreqModLFO,    sf::Generator::DelayVibLFO,
                                         sf::Generator::FreqVibLFO,    sf::Generator::DelayModEnv,   sf::Generator::AttackModEnv,  sf::Generator::HoldModEnv,
                                         sf::Generator::DecayModEnv,   sf::Generator::SustainModEnv, sf::Generator::ReleaseModEnv, sf::Generator::DelayVolEnv,
                                         sf::Generator::AttackVolEnv,  sf::Generator::HoldVolEnv,    sf::Generator::DecayVolEnv,   sf::Generator::SustainVolEnv,
                                         sf::Generator::ReleaseVolEnv, sf::Generator::CoarseTune};

    for (const auto &generator : INIT_GENERATORS) {
        updateModulatedParams(generator);
    }
}

std::size_t Voice::getNoteID() const {
    return noteID_;
}

std::uint8_t Voice::getActualKey() const {
    return actualKey_;
}

std::int16_t Voice::getExclusiveClass() const {
    return generators_.getOrDefault(sf::Generator::ExclusiveClass);
}

const Voice::State &Voice::getStatus() const {
    return status_;
}

StereoValue Voice::render() const {
    const std::uint32_t i = index_.getIntegerPart();
    const double r = index_.getFractionalPart();
    const double interpolated = (1.0 - r) * sampleBuffer_.at(i) + r * sampleBuffer_.at(i + 1);
    return amp_ * volume_ * (interpolated / INT16_MAX);
}

void Voice::setPercussion(bool percussion) {
    percussion_ = percussion;
}

void Voice::updateSFController(sf::GeneralController controller, double value) {
    for (auto &mod : modulators_) {
        if (mod.updateSFController(controller, value)) {
            updateModulatedParams(mod.getDestination());
        }
    }
}

void Voice::updateMIDIController(std::uint8_t controller, std::uint8_t value) {
    for (auto &mod : modulators_) {
        if (mod.updateMIDIController(controller, value)) {
            updateModulatedParams(mod.getDestination());
        }
    }
}

void Voice::updateFineTuning(double fineTuning) {
    fineTuning_ = fineTuning;
    updateModulatedParams(sf::Generator::FineTune);
}

void Voice::updateCoarseTuning(double coarseTuning) {
    coarseTuning_ = coarseTuning;
    updateModulatedParams(sf::Generator::CoarseTune);
}

void Voice::release(bool sustained) {
    if (status_ != State::Playing && status_ != State::Sustained) {
        return;
    }

    if (sustained) {
        status_ = State::Sustained;
    } else {
        status_ = State::Released;
        volEnv_.release();
        modEnv_.release();
    }
}

void Voice::update() {
    const bool calc = steps_++ % CALC_INTERVAL == 0;

    if (calc) {
        // dynamic range of signed 16 bit samples in centibel
        static const double DYNAMIC_RANGE = 200.0 * std::log10(INT16_MAX + 1.0);
        if (volEnv_.getPhase() == Envelope::Phase::Finished ||
            (volEnv_.getPhase() > Envelope::Phase::Attack && minAtten_ + 960.0 * (1.0 - volEnv_.getValue()) >= DYNAMIC_RANGE)) {
            status_ = State::Finished;
            return;
        }

        volEnv_.update();
    }

    index_ += deltaIndex_;

    switch (rtSample_.mode) {
    case SampleMode::UnLooped:
    case SampleMode::UnUsed:
        if (index_.getIntegerPart() >= rtSample_.end) {
            status_ = State::Finished;
            return;
        }
        break;
    case SampleMode::Looped:
        if (index_.getIntegerPart() >= rtSample_.endLoop) {
            index_ -= FixedPoint(rtSample_.endLoop - rtSample_.startLoop);
        }
        break;
    case SampleMode::LoopedUntilRelease:
        if (status_ == State::Released) {
            if (index_.getIntegerPart() >= rtSample_.end) {
                status_ = State::Finished;
                return;
            }
        } else if (index_.getIntegerPart() >= rtSample_.endLoop) {
            index_ -= FixedPoint(rtSample_.endLoop - rtSample_.startLoop);
        }
        break;
    default:
        throw std::runtime_error("unknown sample mode");
    }

    amp_ += deltaAmp_;

    if (calc) {
        modEnv_.update();
        vibLFO_.update();
        modLFO_.update();

        const double modEnvValue = modEnv_.getPhase() == Envelope::Phase::Attack ? conv::convex(modEnv_.getValue()) : modEnv_.getValue();
        const double pitch = voicePitch_ + 0.01 * (getModulatedGenerator(sf::Generator::ModEnvToPitch) * modEnvValue +
                                                   getModulatedGenerator(sf::Generator::VibLfoToPitch) * vibLFO_.getValue() +
                                                   getModulatedGenerator(sf::Generator::ModLfoToPitch) * modLFO_.getValue());
        deltaIndex_ = FixedPoint(deltaIndexRatio_ * conv::keyToHertz(pitch));

        const double attenModLFO = getModulatedGenerator(sf::Generator::ModLfoToVolume) * modLFO_.getValue();
        const double targetAmp = volEnv_.getPhase() == Envelope::Phase::Attack ? volEnv_.getValue() * conv::attenuationToAmplitude(attenModLFO)
                                                                               : conv::attenuationToAmplitude(960.0 * (1.0 - volEnv_.getValue()) + attenModLFO);
        deltaAmp_ = (targetAmp - amp_) / CALC_INTERVAL;
    }
}

double Voice::getModulatedGenerator(sf::Generator type) const {
    return modulated_.at(static_cast<std::size_t>(type));
}

StereoValue calculatePannedVolume(double pan) {
    if (pan <= -500.0) {
        return {1.0, 0.0};
    } else if (pan >= 500.0) {
        return {0.0, 1.0};
    } else {
        static constexpr double FACTOR = 3.141592653589793 / 2000.0;
        return {std::sin(FACTOR * (-pan + 500.0)), std::sin(FACTOR * (pan + 500.0))};
    }
}

void Voice::updateModulatedParams(sf::Generator destination) {
    double &modulated = modulated_.at(static_cast<std::size_t>(destination));
    modulated = generators_.getOrDefault(destination);
    if (destination == sf::Generator::InitialAttenuation) {
        modulated *= ATTEN_FACTOR;
    }
    for (const auto &mod : modulators_) {
        if (mod.getDestination() == destination) {
            modulated += mod.getValue();
        }
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
    switch (destination) {
    case sf::Generator::Pan:
    case sf::Generator::InitialAttenuation:
        volume_ = conv::attenuationToAmplitude(getModulatedGenerator(sf::Generator::InitialAttenuation)) *
                  calculatePannedVolume(getModulatedGenerator(sf::Generator::Pan));
        break;
    case sf::Generator::DelayModLFO:
        modLFO_.setDelay(modulated);
        break;
    case sf::Generator::FreqModLFO:
        modLFO_.setFrequency(modulated);
        break;
    case sf::Generator::DelayVibLFO:
        vibLFO_.setDelay(modulated);
        break;
    case sf::Generator::FreqVibLFO:
        vibLFO_.setFrequency(modulated);
        break;
    case sf::Generator::DelayModEnv:
        modEnv_.setParameter(Envelope::Phase::Delay, modulated);
        break;
    case sf::Generator::AttackModEnv:
        modEnv_.setParameter(Envelope::Phase::Attack, modulated);
        break;
    case sf::Generator::HoldModEnv:
    case sf::Generator::KeynumToModEnvHold:
        modEnv_.setParameter(Envelope::Phase::Hold,
                             getModulatedGenerator(sf::Generator::HoldModEnv) + getModulatedGenerator(sf::Generator::KeynumToModEnvHold) * keyScaling_);
        break;
    case sf::Generator::DecayModEnv:
    case sf::Generator::KeynumToModEnvDecay:
        modEnv_.setParameter(Envelope::Phase::Decay,
                             getModulatedGenerator(sf::Generator::DecayModEnv) + getModulatedGenerator(sf::Generator::KeynumToModEnvDecay) * keyScaling_);
        break;
    case sf::Generator::SustainModEnv:
        modEnv_.setParameter(Envelope::Phase::Sustain, modulated);
        break;
    case sf::Generator::ReleaseModEnv:
        modEnv_.setParameter(Envelope::Phase::Release, modulated);
        break;
    case sf::Generator::DelayVolEnv:
        volEnv_.setParameter(Envelope::Phase::Delay, modulated);
        break;
    case sf::Generator::AttackVolEnv:
        volEnv_.setParameter(Envelope::Phase::Attack, modulated);
        break;
    case sf::Generator::HoldVolEnv:
    case sf::Generator::KeynumToVolEnvHold:
        volEnv_.setParameter(Envelope::Phase::Hold,
                             getModulatedGenerator(sf::Generator::HoldVolEnv) + getModulatedGenerator(sf::Generator::KeynumToVolEnvHold) * keyScaling_);
        break;
    case sf::Generator::DecayVolEnv:
    case sf::Generator::KeynumToVolEnvDecay:
        volEnv_.setParameter(Envelope::Phase::Decay,
                             getModulatedGenerator(sf::Generator::DecayVolEnv) + getModulatedGenerator(sf::Generator::KeynumToVolEnvDecay) * keyScaling_);
        break;
    case sf::Generator::SustainVolEnv:
        volEnv_.setParameter(Envelope::Phase::Sustain, modulated);
        break;
    case sf::Generator::ReleaseVolEnv:
        volEnv_.setParameter(Envelope::Phase::Release, modulated);
        break;
    case sf::Generator::CoarseTune:
    case sf::Generator::FineTune:
    case sf::Generator::ScaleTuning:
    case sf::Generator::Pitch:
        voicePitch_ = rtSample_.pitch + 0.01 * getModulatedGenerator(sf::Generator::Pitch) +
                      0.01 * generators_.getOrDefault(sf::Generator::ScaleTuning) * (actualKey_ - rtSample_.pitch) + coarseTuning_ +
                      getModulatedGenerator(sf::Generator::CoarseTune) + 0.01 * (fineTuning_ + getModulatedGenerator(sf::Generator::FineTune));
        break;
    }
#pragma GCC diagnostic pop
}

Channel::Channel(double outputRate)
    : outputRate_(outputRate), controllers_(), rpns_(), keyPressures_(), channelPressure_(0), pitchBend_(1 << 13), dataEntryMode_(DataEntryMode::RPN),
      pitchBendSensitivity_(2.0), fineTuning_(0.0), coarseTuning_(0.0), currentNoteID_(0) {
    controllers_.at(static_cast<std::size_t>(midi::ControlChange::Volume)) = 100;
    controllers_.at(static_cast<std::size_t>(midi::ControlChange::Pan)) = 64;
    controllers_.at(static_cast<std::size_t>(midi::ControlChange::Expression)) = 127;
    controllers_.at(static_cast<std::size_t>(midi::ControlChange::RPNLSB)) = 127;
    controllers_.at(static_cast<std::size_t>(midi::ControlChange::RPNMSB)) = 127;
    voices_.reserve(128);
}

midi::Bank Channel::getBank() const {
    return {controllers_.at(static_cast<std::size_t>(midi::ControlChange::BankSelectMSB)),
            controllers_.at(static_cast<std::size_t>(midi::ControlChange::BankSelectLSB))};
}

bool Channel::hasPreset() const {
    return static_cast<bool>(preset_);
}

void Channel::noteOff(std::uint8_t key) {
    const bool sustained = controllers_.at(static_cast<std::size_t>(midi::ControlChange::Sustain)) >= 64;

    for (const auto &voice : voices_) {
        if (voice->getActualKey() == key) {
            voice->release(sustained);
        }
    }
}

void Channel::noteOn(std::uint8_t key, std::uint8_t velocity) {
    if (velocity == 0) {
        noteOff(key);
        return;
    }

    for (const Zone &presetZone : preset_->zones) {
        if (presetZone.isInRange(key, velocity)) {
            const std::int16_t instID = presetZone.generators.getOrDefault(sf::Generator::Instrument);
            const auto &inst = preset_->soundFont.getInstruments().at(instID);
            for (const Zone &instZone : inst.zones) {
                if (instZone.isInRange(key, velocity)) {
                    const std::int16_t sampleID = instZone.generators.getOrDefault(sf::Generator::SampleID);
                    const auto &sample = preset_->soundFont.getSamples().at(sampleID);

                    auto generators = instZone.generators;
                    generators.add(presetZone.generators);

                    auto modparams = instZone.modulatorParameters;
                    modparams.mergeAndAdd(presetZone.modulatorParameters);
                    modparams.merge(ModulatorParameterSet::getDefaultParameters());

                    auto voice = std::make_unique<Voice>(currentNoteID_, outputRate_, sample, generators, modparams, key, velocity);
                    voice->setPercussion(preset_->bank == PERCUSSION_BANK);
                    addVoice(std::move(voice));
                }
            }
        }
    }
    ++currentNoteID_;
}

void Channel::keyPressure(std::uint8_t key, std::uint8_t value) {
    keyPressures_.at(key) = value;

    for (const auto &voice : voices_) {
        if (voice->getActualKey() == key) {
            voice->updateSFController(sf::GeneralController::PolyPressure, value);
        }
    }
}

void Channel::controlChange(std::uint8_t controller, std::uint8_t value) {
    controllers_.at(controller) = value;

    switch (static_cast<midi::ControlChange>(controller)) {
    case midi::ControlChange::DataEntryMSB:
    case midi::ControlChange::DataEntryLSB:
        if (dataEntryMode_ == DataEntryMode::RPN) {
            const std::uint16_t rpn = getSelectedRPN();
            if (rpn < static_cast<std::uint16_t>(midi::RPN::Last)) {
                const std::uint16_t data = midi::joinBytes(controllers_.at(static_cast<std::size_t>(midi::ControlChange::DataEntryMSB)),
                                                           controllers_.at(static_cast<std::size_t>(midi::ControlChange::DataEntryLSB)));
                rpns_.at(rpn) = data;
                updateRPN();
            }
        }
        break;
    case midi::ControlChange::Sustain:
        if (value < 64) {
            for (const auto &voice : voices_) {
                if (voice->getStatus() == Voice::State::Sustained) {
                    voice->release(false);
                }
            }
        }
        break;
    case midi::ControlChange::DataIncrement:
        if (dataEntryMode_ == DataEntryMode::RPN) {
            const std::uint16_t rpn = getSelectedRPN();
            if (rpn < static_cast<std::uint16_t>(midi::RPN::Last) && rpns_.at(rpn) >> 7 < 127) {
                rpns_.at(rpn) += 1 << 7;
                updateRPN();
            }
        }
        break;
    case midi::ControlChange::DataDecrement:
        if (dataEntryMode_ == DataEntryMode::RPN) {
            const std::uint16_t rpn = getSelectedRPN();
            if (rpn < static_cast<std::uint16_t>(midi::RPN::Last) && rpns_.at(rpn) >> 7 > 0) {
                rpns_.at(rpn) -= 1 << 7;
                updateRPN();
            }
        }
        break;
    case midi::ControlChange::NRPNMSB:
    case midi::ControlChange::NRPNLSB:
        dataEntryMode_ = DataEntryMode::NRPN;
        break;
    case midi::ControlChange::RPNMSB:
    case midi::ControlChange::RPNLSB:
        dataEntryMode_ = DataEntryMode::RPN;
        break;
    case midi::ControlChange::AllSoundOff:
        voices_.clear();
        break;
    case midi::ControlChange::ResetAllControllers:
        // See "General MIDI System Level 1 Developer Guidelines" Second Revision
        // p.5 'Response to "Reset All Controllers" Message'
        keyPressures_ = {};
        channelPressure_ = 0;
        pitchBend_ = 1 << 13;
        for (const auto &voice : voices_) {
            voice->updateSFController(sf::GeneralController::ChannelPressure, channelPressure_);
            voice->updateSFController(sf::GeneralController::PitchWheel, pitchBend_);
        }
        for (std::uint8_t i = 1; i < 122; ++i) {
            if ((91 <= i && i <= 95) || (70 <= i && i <= 79)) {
                continue;
            }
            switch (static_cast<midi::ControlChange>(i)) {
            case midi::ControlChange::Volume:
            case midi::ControlChange::Pan:
            case midi::ControlChange::BankSelectLSB:
            case midi::ControlChange::AllSoundOff:
                break;
            case midi::ControlChange::Expression:
            case midi::ControlChange::RPNLSB:
            case midi::ControlChange::RPNMSB:
                controllers_.at(i) = 127;
                for (const auto &voice : voices_) {
                    voice->updateMIDIController(i, 127);
                }
                break;
            default:
                controllers_.at(i) = 0;
                for (const auto &voice : voices_) {
                    voice->updateMIDIController(i, 0);
                }
                break;
            }
        }
        break;
    case midi::ControlChange::AllNotesOff: {
        // See "The Complete MIDI 1.0 Detailed Specification" Rev. April 2006
        // p.A-6 'The Relationship Between the Hold Pedal and "All Notes Off"'

        // All Notes Off is affected by CC 64 (Sustain)
        const bool sustained = controllers_.at(static_cast<std::size_t>(midi::ControlChange::Sustain)) >= 64;
        for (const auto &voice : voices_) {
            voice->release(sustained);
        }
        break;
    }
    default:
        for (const auto &voice : voices_) {
            voice->updateMIDIController(controller, value);
        }
        break;
    }
}

void Channel::channelPressure(std::uint8_t value) {
    channelPressure_ = value;
    for (const auto &voice : voices_) {
        voice->updateSFController(sf::GeneralController::ChannelPressure, value);
    }
}

void Channel::pitchBend(std::uint16_t value) {
    pitchBend_ = value;
    for (const auto &voice : voices_) {
        voice->updateSFController(sf::GeneralController::PitchWheel, value);
    }
}

void Channel::setPreset(const std::shared_ptr<const Preset> &preset) {
    preset_ = preset;
}

StereoValue Channel::render() {
    StereoValue sum{0.0, 0.0};
    for (const auto &voice : voices_) {
        if (voice->getStatus() == Voice::State::Finished) {
            continue;
        }
        voice->update();

        if (voice->getStatus() == Voice::State::Finished) {
            continue;
        }
        sum += voice->render();
    }
    return sum;
}

std::uint16_t Channel::getSelectedRPN() const {
    return midi::joinBytes(controllers_.at(static_cast<std::size_t>(midi::ControlChange::RPNMSB)),
                           controllers_.at(static_cast<std::size_t>(midi::ControlChange::RPNLSB)));
}

void Channel::addVoice(std::unique_ptr<Voice> voice) {
    voice->updateSFController(sf::GeneralController::PolyPressure, keyPressures_.at(voice->getActualKey()));
    voice->updateSFController(sf::GeneralController::ChannelPressure, channelPressure_);
    voice->updateSFController(sf::GeneralController::PitchWheel, pitchBend_);
    voice->updateSFController(sf::GeneralController::PitchWheelSensitivity, pitchBendSensitivity_);
    voice->updateFineTuning(fineTuning_);
    voice->updateCoarseTuning(coarseTuning_);
    for (std::uint8_t i = 0; i < midi::NUM_CONTROLLERS; ++i) {
        voice->updateMIDIController(i, controllers_.at(i));
    }

    const auto exclusiveClass = voice->getExclusiveClass();

    if (exclusiveClass != 0) {
        for (const auto &v : voices_) {
            if (v->getNoteID() != currentNoteID_ && v->getExclusiveClass() == exclusiveClass) {
                v->release(false);
            }
        }
    }

    for (auto &v : voices_) {
        if (v->getStatus() == Voice::State::Finished) {
            v = std::move(voice);
            return;
        }
    }
    voices_.emplace_back(std::move(voice));
}

void Channel::updateRPN() {
    const std::uint16_t rpn = getSelectedRPN();
    const auto data = static_cast<std::int32_t>(rpns_.at(rpn));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
    switch (static_cast<midi::RPN>(rpn)) {
    case midi::RPN::PitchBendSensitivity:
        pitchBendSensitivity_ = data / 128.0;
        for (const auto &voice : voices_) {
            voice->updateSFController(sf::GeneralController::PitchWheelSensitivity, pitchBendSensitivity_);
        }
        break;
    case midi::RPN::FineTuning: {
        fineTuning_ = (data - 8192) / 81.92;
        for (const auto &voice : voices_) {
            voice->updateFineTuning(fineTuning_);
        }
        break;
    }
    case midi::RPN::CoarseTuning: {
        coarseTuning_ = (data - 8192) / 128.0;
        for (const auto &voice : voices_) {
            voice->updateCoarseTuning(coarseTuning_);
        }
        break;
    }
    }
#pragma GCC diagnostic pop
}

// Used when dealing with soundfonts that are missing some expected default presets
static bool no_drums = false;
static bool no_piano = false;

Synthesizer::Synthesizer(double outputRate, std::size_t numChannels)
    : midiStd_(midi::Standard::GM), defaultMIDIStd_(midi::Standard::GM), stdFixed_(false), volume_(1.0) {
    conv::initialize();

    channels_.reserve(numChannels);
    for (std::size_t i = 0; i < numChannels; ++i) {
        channels_.emplace_back(std::make_unique<Channel>(outputRate));
    }

    no_drums = false;
    no_piano = false;
}

void Synthesizer::render_float(float *buffer, size_t samples) {
    for (std::size_t samp = 0; samp < samples; samp += 2) {
        StereoValue sum{0.0, 0.0};
        for (const auto &channel : channels_) {
            sum += channel->render();
        }
        sum = sum * volume_;
        buffer[samp] = sum.left;
        buffer[samp + 1] = sum.right;
    }
}

void Synthesizer::render_s16(int16_t *buffer, size_t samples) {
    for (std::size_t samp = 0; samp < samples; samp += 2) {
        StereoValue sum{0.0, 0.0};
        for (const auto &channel : channels_) {
            sum += channel->render();
        }
        buffer[samp] = (int16_t)((int)(sum.left * volume_ * 32767.5f) & 0xFFFF);
        buffer[samp + 1] = (int16_t)((int)(sum.right * volume_ * 32767.5f) & 0xFFFF);
    }
}

void Synthesizer::loadSoundFont(const std::string &filename) {
    soundFonts_.emplace_back(std::make_unique<SoundFont>(filename));
}

void Synthesizer::setVolume(double volume) {
    volume_ = std::max(0.0, volume);
}

void Synthesizer::setMIDIStandard(midi::Standard midiStandard, bool fixed) {
    midiStd_ = midiStandard;
    defaultMIDIStd_ = midiStandard;
    stdFixed_ = fixed;
}

template <std::size_t N> bool matchSysEx(const char *data, std::size_t length, const std::array<unsigned char, N> &sysEx) {
    if (length != N) {
        return false;
    }

    for (std::size_t i = 0; i < N; ++i) {
        if (i == 2) {
            // respond to all device IDs
            continue;
        } else if (data[i] != static_cast<char>(sysEx.at(i))) {
            return false;
        }
    }
    return true;
}

void Synthesizer::processSysEx(const char *data, std::size_t length) {
    static constexpr std::array<unsigned char, 6> GM_SYSTEM_ON = {0xf0, 0x7e, 0, 0x09, 0x01, 0xf7};
    static constexpr std::array<unsigned char, 6> GM_SYSTEM_OFF = {0xf0, 0x7e, 0, 0x09, 0x02, 0xf7};
    static constexpr std::array<unsigned char, 11> GS_RESET = {0xf0, 0x41, 0, 0x42, 0x12, 0x40, 0x00, 0x7f, 0x00, 0x41, 0xf7};
    static constexpr std::array<unsigned char, 11> GS_SYSTEM_MODE_SET1 = {0xf0, 0x41, 0, 0x42, 0x12, 0x00, 0x00, 0x7f, 0x00, 0x01, 0xf7};
    static constexpr std::array<unsigned char, 11> GS_SYSTEM_MODE_SET2 = {0xf0, 0x41, 0, 0x42, 0x12, 0x00, 0x00, 0x7f, 0x01, 0x00, 0xf7};
    static constexpr std::array<unsigned char, 9> XG_SYSTEM_ON = {0xf0, 0x43, 0, 0x4c, 0x00, 0x00, 0x7e, 0x00, 0xf7};

    if (stdFixed_) {
        return;
    }
    if (matchSysEx(data, length, GM_SYSTEM_ON)) {
        midiStd_ = midi::Standard::GM;
    } else if (matchSysEx(data, length, GM_SYSTEM_OFF)) {
        midiStd_ = defaultMIDIStd_;
    } else if (matchSysEx(data, length, GS_RESET) || matchSysEx(data, length, GS_SYSTEM_MODE_SET1) || matchSysEx(data, length, GS_SYSTEM_MODE_SET2)) {
        midiStd_ = midi::Standard::GS;
    } else if (matchSysEx(data, length, XG_SYSTEM_ON)) {
        midiStd_ = midi::Standard::XG;
    }
}

std::shared_ptr<const Preset> Synthesizer::findPreset(std::uint16_t bank, std::uint16_t presetID) const {
    for (const auto &sf : soundFonts_) {
        for (const auto &preset : sf->getPresetPtrs()) {
            if (preset->bank == bank && preset->presetID == presetID) {
                return preset;
            }
        }
    }

    // fallback
    if (bank == PERCUSSION_BANK) {
        if (presetID != 0) {
            // fall back to GM percussion
            return findPreset(bank, 0);
        } else {
            // throw std::runtime_error("failed to find preset 128:0 (GM Percussion)");
            no_drums = true;
            return nullptr;
        }
    } else if (bank != 0) {
        // fall back to GM bank
        return findPreset(0, presetID);
    } else if (presetID != 0) {
        // preset not found even in GM bank, fall back to Piano
        return findPreset(0, 0);
    } else {
        // Piano not found, there is no more fallback
        // throw std::runtime_error("failed to find preset 0:0 (GM Acoustic Grand Piano)");
        no_piano = true;
        return nullptr;
    }
}

void Synthesizer::processChannelMessage(midi::MessageStatus event, std::uint8_t chan, std::uint8_t param1, std::uint8_t param2) {

    const auto &channel = channels_.at(chan);

    switch (event) {
    case midi::MessageStatus::NoteOff:
        channel->noteOff(param1);
        break;
    case midi::MessageStatus::NoteOn:
        if (!channel->hasPreset()) {
            if (chan == midi::PERCUSSION_CHANNEL) {
                if (!no_drums) {
                    channel->setPreset(findPreset(PERCUSSION_BANK, 0));
                    if (!no_drums)
                        channel->noteOn(param1, param2);
                    else
                        return;
                } else
                    return;
            } else {
                if (!no_piano) {
                    channel->setPreset(findPreset(0, 0));
                    if (!no_piano)
                        channel->noteOn(param1, param2);
                    else
                        return;
                } else
                    return;
            }
        }
        channel->noteOn(param1, param2);
        break;
    case midi::MessageStatus::KeyPressure:
        channel->keyPressure(param1, param2);
        break;
    case midi::MessageStatus::ControlChange:
        channel->controlChange(param1, param2);
        break;
    case midi::MessageStatus::ProgramChange: {
        const auto midiBank = channel->getBank();
        std::uint16_t sfBank = 0;
        switch (midiStd_) {
        case midi::Standard::GM:
            break;
        case midi::Standard::GS:
            sfBank = midiBank.msb;
            break;
        case midi::Standard::XG:
            // assuming no one uses XG voices bank MSBs of which overlap normal voices' bank LSBs
            // e.g. SFX voice (MSB=64)
            sfBank = midiBank.msb == 127 ? PERCUSSION_BANK : midiBank.lsb;
            break;
        default:
            throw std::runtime_error("unknown MIDI standard");
        }
        channel->setPreset(findPreset(chan == midi::PERCUSSION_CHANNEL ? PERCUSSION_BANK : sfBank, param1));
        break;
    }
    case midi::MessageStatus::ChannelPressure:
        channel->channelPressure(param1);
        break;
    case midi::MessageStatus::PitchBend:
        channel->pitchBend(midi::joinBytes(param2, param1));
        break;
    }
}

void Synthesizer::pause() {
    for (std::size_t chan = 0; chan < channels_.size(); chan++) {
        channels_.at(chan)->controlChange(123, 0); // AllNotesOff
    }
}

void Synthesizer::stop() {
    for (std::size_t chan = 0; chan < channels_.size(); chan++) {
        channels_.at(chan)->controlChange(120, 0); // AllSoundOff
    }
}

uint32_t Synthesizer::getActiveVoiceCount() const {
    uint32_t totalVoices = 0;

    for (auto &chan : channels_) {
        // if (voice.channel && (voice.on || voice.justChanged))
        for (auto &voice : chan->voices_) {
            if (voice->getStatus() != Voice::State::Finished)
                ++totalVoices;
        }
    }

    return totalVoices;
}
} // namespace primesynth
