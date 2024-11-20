
/** $VER: Support.h (2024.05.03) P. Stuer **/

#pragma once

#include <algorithm>

/// <summary>
/// Returns true of the input value is in the interval between min and max.
/// </summary>
template <class T> inline constexpr static T InRange(T value, T minValue, T maxValue) {
    return (minValue <= value) && (value <= maxValue);
}
