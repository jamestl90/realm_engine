#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <cassert>

namespace memory {

// Fixed-size block pool allocator
template<typename T>
class PoolAllocator {
public:
    explicit PoolAllocator(std::size_t initial_capacity = 256) {
        reserve(initial_capacity);
    }

    ~PoolAllocator() {
        clear();
    }

    // Non-copyable, movable
    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;
    PoolAllocator(PoolAllocator&&) noexcept = default;
    PoolAllocator& operator=(PoolAllocator&&) noexcept = default;

    // Allocate object
    template<typename... Args>
    [[nodiscard]] T* allocate(Args&&... args) {
        if (free_list_.empty()) {
            grow();
        }

        T* ptr = free_list_.back();
        free_list_.pop_back();
        new (ptr) T(std::forward<Args>(args)...);
        return ptr;
    }

    // Deallocate object
    void deallocate(T* ptr) noexcept {
        if (!ptr) return;
        ptr->~T();
        free_list_.push_back(ptr);
    }

    // Reserve capacity
    void reserve(std::size_t capacity) {
        while (blocks_.size() * block_size_ < capacity) {
            grow();
        }
    }

    // Clear all allocations
    void clear() noexcept {
        for (auto* block : blocks_) {
            ::operator delete(block);
        }
        blocks_.clear();
        free_list_.clear();
    }

    [[nodiscard]] std::size_t capacity() const noexcept {
        return blocks_.size() * block_size_;
    }

private:
    void grow() {
        constexpr std::size_t alignment = alignof(T);
        void* block = ::operator new(block_size_ * sizeof(T), std::align_val_t{alignment});
        blocks_.push_back(static_cast<T*>(block));

        // Add all slots to free list
        T* ptr = static_cast<T*>(block);
        for (std::size_t i = 0; i < block_size_; ++i) {
            free_list_.push_back(ptr + i);
        }
    }

    static constexpr std::size_t block_size_ = 256;
    std::vector<T*> blocks_;
    std::vector<T*> free_list_;
};

} // namespace memory
