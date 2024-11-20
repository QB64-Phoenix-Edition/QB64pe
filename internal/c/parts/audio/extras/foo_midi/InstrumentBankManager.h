//----------------------------------------------------------------------------------------------------------------------
// foo_midi: A foobar2000 component to play MIDI files (https://github.com/stuerp/foo_midi)
// Copyright (c) 2022-2024 Peter Stuer
//
// This file was written from scratch for QB64-PE
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "filepath.h"
#include <cstdint>
#include <string>
#include <vector>

class InstrumentBankManager {
  public:
    enum class Type : uint32_t { Opal, Primesynth, TinySoundFont, VSTi };

    enum class Location : uint32_t { Memory, File };

    InstrumentBankManager() {
        SetDefaults();
    };

    auto GetType() {
        return type;
    }

    auto GetLocation() {
        return location;
    }

    void SetPath(const char *path);

    auto GetPath() {
        return fileName.c_str();
    }

    void SetData(const uint8_t *data, size_t size, Type type = Type::Opal);

    auto GetData() {
        return data.data();
    }

    auto GetDataSize() {
        return data.size();
    }

  private:
    static const uint8_t defaultBank[];
    static const size_t defaultBankSize = 212069;

    Type type;
    Location location;
    std::string fileName;
    std::vector<uint8_t> data;

    void SetDefaults();
};
