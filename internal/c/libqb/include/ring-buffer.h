#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

/// @brief A fixed-capacity ring buffer designed for single-producer single-consumer use. Head is owned exclusively by the producer; tail is owned exclusively
/// by the consumer.
/// @tparam T The type of elements stored in the buffer. Must be trivially copyable if `OverwriteOnFull` is true, since the implementation may overwrite unread
/// elements when the buffer is full.
/// @tparam Capacity Must be a power of two and >= 2. The usable capacity of the buffer is actually Capacity - 1, since one slot is used to distinguish between
/// full and empty states.
/// @tparam OverwriteOnFull If false, `Push` will fail when the buffer is full. If true, `Push` will overwrite the oldest element in the buffer when it is full.
/// When OverwriteOnFull = true, the producer may advance tail to discard the oldest item when the buffer is full. This is NOT safe for general concurrent use.
template <typename T, std::size_t Capacity, bool OverwriteOnFull = false> class RingBuffer {
    static_assert(Capacity >= 2, "Capacity must be >= 2");
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");
    static_assert(!OverwriteOnFull || std::is_trivially_copyable_v<T>, "OverwriteOnFull requires trivially copyable T");

    static constexpr std::size_t Mask = Capacity - 1;

#if defined(__cpp_lib_hardware_interference_size) && !defined(__GNUC__)
    static constexpr std::size_t CacheLineSize = std::hardware_destructive_interference_size;
#else
    static constexpr std::size_t CacheLineSize = 64;
#endif

    struct alignas(CacheLineSize) AtomicIndex {
        std::atomic<std::size_t> value{0};
    };

  public:
    RingBuffer() : buffer(std::make_unique<T[]>(Capacity)) {}

    RingBuffer(const RingBuffer &) = delete;
    RingBuffer &operator=(const RingBuffer &) = delete;

    [[nodiscard]]
    static constexpr std::size_t GetCapacity() noexcept {
        return Capacity - 1;
    }

    template <typename U>
    [[nodiscard("`Push` can fail when `OverwriteOnFull` is false — check the return value")]]
    bool Push(U &&value) noexcept(std::is_nothrow_assignable_v<T &, U &&>)
    requires(!OverwriteOnFull)
    {
        const auto headIndex = head.value.load(std::memory_order_relaxed);
        const auto next = Increment(headIndex);

        if (next == tail.value.load(std::memory_order_acquire)) {
            return false;
        }

        buffer[headIndex] = std::forward<U>(value);
        head.value.store(next, std::memory_order_release);
        return true;
    }

    template <typename U>
    bool Push(U &&value) noexcept(std::is_nothrow_assignable_v<T &, U &&>)
    requires(OverwriteOnFull)
    {
        const auto headIndex = head.value.load(std::memory_order_relaxed);
        const auto tailIndex = tail.value.load(std::memory_order_acquire);
        const auto next = Increment(headIndex);

        if (next == tailIndex) {
            tail.value.store(Increment(tailIndex), std::memory_order_release);
        }

        buffer[headIndex] = std::forward<U>(value);
        head.value.store(next, std::memory_order_release);
        return true;
    }

    template <typename U>
    [[nodiscard("`TryPush` can fail if the buffer is full — check the return value")]]
    bool TryPush(U &&value) noexcept(std::is_nothrow_assignable_v<T &, U &&>)
    requires(OverwriteOnFull)
    {
        const auto headIndex = head.value.load(std::memory_order_relaxed);
        const auto next = Increment(headIndex);

        if (next == tail.value.load(std::memory_order_acquire)) {
            return false;
        }

        buffer[headIndex] = std::forward<U>(value);
        head.value.store(next, std::memory_order_release);
        return true;
    }

    [[nodiscard]]
    bool Pop(T &out) noexcept(std::is_nothrow_move_assignable_v<T>) {
        const auto tailIndex = tail.value.load(std::memory_order_relaxed);

        if (tailIndex == head.value.load(std::memory_order_acquire)) {
            return false;
        }

        out = std::move(buffer[tailIndex]);
        tail.value.store(Increment(tailIndex), std::memory_order_release);
        return true;
    }

    [[nodiscard]]
    const T *PeekFront() const noexcept {
        const auto tailIndex = tail.value.load(std::memory_order_relaxed);

        if (tailIndex == head.value.load(std::memory_order_acquire)) {
            return nullptr;
        }

        return &buffer[tailIndex];
    }

    [[nodiscard]]
    T *PeekFront() noexcept {
        const auto tailIndex = tail.value.load(std::memory_order_relaxed);

        if (tailIndex == head.value.load(std::memory_order_acquire)) {
            return nullptr;
        }

        return &buffer[tailIndex];
    }

    [[nodiscard]]
    const T *PeekBack() const noexcept {
        const auto headIndex = head.value.load(std::memory_order_acquire);

        if (headIndex == tail.value.load(std::memory_order_relaxed)) {
            return nullptr;
        }

        return &buffer[(headIndex - 1) & Mask];
    }

    [[nodiscard]]
    T *PeekBack() noexcept {
        const auto headIndex = head.value.load(std::memory_order_acquire);

        if (headIndex == tail.value.load(std::memory_order_relaxed)) {
            return nullptr;
        }

        return &buffer[(headIndex - 1) & Mask];
    }

    void Clear() noexcept {
        tail.value.store(head.value.load(std::memory_order_acquire), std::memory_order_release);
    }

    [[nodiscard]]
    bool IsEmpty() const noexcept {
        return head.value.load(std::memory_order_acquire) == tail.value.load(std::memory_order_acquire);
    }

    [[nodiscard]]
    bool IsFull() const noexcept {
        return Increment(head.value.load(std::memory_order_acquire)) == tail.value.load(std::memory_order_acquire);
    }

    [[nodiscard]]
    std::size_t GetSize() const noexcept {
        return (head.value.load(std::memory_order_acquire) - tail.value.load(std::memory_order_acquire)) & Mask;
    }

  private:
    static constexpr std::size_t Increment(std::size_t i) noexcept {
        return (i + 1) & Mask;
    }

    AtomicIndex head;
    AtomicIndex tail;

    std::unique_ptr<T[]> buffer;
};
