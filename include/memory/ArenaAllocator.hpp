#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <cassert>

namespace memory {

// Linear arena allocator - fast allocation, bulk deallocation
class ArenaAllocator {
public:
    explicit ArenaAllocator(std::size_t capacity);
    ~ArenaAllocator();

    // Non-copyable, movable
    ArenaAllocator(const ArenaAllocator&) = delete;
    ArenaAllocator& operator=(const ArenaAllocator&) = delete;
    ArenaAllocator(ArenaAllocator&&) noexcept;
    ArenaAllocator& operator=(ArenaAllocator&&) noexcept;

    // Allocate aligned memory
    [[nodiscard]] void* allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t));

    // Typed allocation
    template<typename T, typename... Args>
    [[nodiscard]] T* create(Args&&... args) {
        void* ptr = allocate(sizeof(T), alignof(T));
        return new (ptr) T(std::forward<Args>(args)...);
    }

    // Reset arena (invalidates all previous allocations)
    void reset() noexcept;

    // Query state
    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
    [[nodiscard]] std::size_t used() const noexcept { return offset_; }
    [[nodiscard]] std::size_t available() const noexcept { return capacity_ - offset_; }

private:
    std::byte* buffer_{nullptr};
    std::size_t capacity_{0};
    std::size_t offset_{0};
};

// RAII scope guard for arena reset
class ArenaScope {
public:
    explicit ArenaScope(ArenaAllocator& arena) noexcept 
        : arena_(arena), saved_offset_(arena.used()) {}

    ~ArenaScope() {
        // Reset to saved offset (partial reset)
        // Note: Requires ArenaAllocator to support this
    }

    ArenaScope(const ArenaScope&) = delete;
    ArenaScope& operator=(const ArenaScope&) = delete;

private:
    ArenaAllocator& arena_;
    std::size_t saved_offset_;
};

} // namespace memory
