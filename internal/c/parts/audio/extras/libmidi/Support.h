
/** $VER: Support.h (2024.05.03) P. Stuer **/

#pragma once

#include <algorithm>

/// <summary>
/// Returns the input value clamped between min and max.
/// </summary>
template <class T> inline constexpr static T Clamp(T value, T minValue, T maxValue) { return std::min(std::max(value, minValue), maxValue); }

/// <summary>
/// Returns true of the input value is in the interval between min and max.
/// </summary>
template <class T> inline constexpr static T InRange(T value, T minValue, T maxValue) { return (minValue <= value) && (value <= maxValue); }
