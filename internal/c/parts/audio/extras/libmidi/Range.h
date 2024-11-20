
/** $VER: Range.h (2024.05.11) P. Stuer **/

#pragma once

#include <climits>
#include <cstdint>

/// <summary>
/// Represents a range.
/// </summary>
class range_t {
  public:
    range_t() noexcept {
        Clear();
    }

    range_t(const range_t &) = delete;

    range_t(range_t &&other) noexcept {
        Set(other._Begin, other._End);
    }

    range_t &operator=(const range_t &) = delete;

    range_t &operator=(range_t &&other) noexcept {
        Set(other._Begin, other._End);
        return *this;
    }

    virtual ~range_t() noexcept {}

    uint32_t Begin() const noexcept {
        return _Begin;
    }

    uint32_t End() const noexcept {
        return _End;
    }

    void SetBegin(uint32_t begin) noexcept {
        _Begin = begin;
    }

    void SetEnd(uint32_t end) noexcept {
        _End = end;
    }

    void Set(uint32_t begin, uint32_t end) noexcept {
        _Begin = begin;
        _End = end;
    }

    void Clear() noexcept {
        Set(std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max());
    }

    uint32_t Size() const noexcept {
        return _End - _Begin;
    }

    bool HasBegin() const noexcept {
        return _Begin != std::numeric_limits<uint32_t>::max();
    }

    bool HasEnd() const noexcept {
        return _End != std::numeric_limits<uint32_t>::max();
    }

    bool IsSet() const noexcept {
        return HasBegin() && HasEnd();
    }

    bool IsEmpty() const noexcept {
        return !IsSet();
    }

  private:
    uint32_t _Begin;
    uint32_t _End;
};
