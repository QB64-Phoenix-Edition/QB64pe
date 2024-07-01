
/** $VER: Exception.h (2024.05.16) P. Stuer **/

#pragma once

#include "framework.h"

class MIDIException : public std::runtime_error {
  public:
    MIDIException(const std::string &errorMessage) : std::runtime_error(errorMessage) {}
};
