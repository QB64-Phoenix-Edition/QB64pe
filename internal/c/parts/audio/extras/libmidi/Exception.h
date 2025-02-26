
/** $VER: Exception.h (2024.08.20) P. Stuer **/

#pragma once

#include "framework.h"

class midi_exception : public std::runtime_error {
  public:
    midi_exception(const std::string &errorMessage) : std::runtime_error(errorMessage) {}
};
