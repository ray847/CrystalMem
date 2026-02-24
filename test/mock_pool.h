#ifndef CRYSTALMEM_TEST_MOCK_POOL_H_
#define CRYSTALMEM_TEST_MOCK_POOL_H_

#include "gmock/gmock.h"
#include "CrystalMem/pool/concept.h" // For AnyPool concept
#include "CrystalMem/vendor/concept.h" // For AnyVendor concept
#include "CrystalMem/type.h" // For size_t etc.

#include <memory> // For std::shared_ptr
#include <utility> // For std::move

namespace crystal::mem {

// Real mock object that holds MOCK_METHODs for AnyVendor
class RealMockVendor {
public:
    MOCK_METHOD(void*, MockAlloc, (size_t size, align_t align));
    MOCK_METHOD(void, MockDealloc, (void* ptr, size_t size, align_t align));

    // Delegate calls from Alloc/Dealloc to mocks
    void* Alloc(size_t size, align_t align) {
        return MockAlloc(size, align);
    }
    void Dealloc(void* ptr, size_t size, align_t align) {
        MockDealloc(ptr, size, align);
    }

    // AnyVendor also requires move_constructible and is_move_assignable_v
    // Using default copy/move to satisfy the concept.
    RealMockVendor() = default;
    RealMockVendor(RealMockVendor&&) = default;
    RealMockVendor& operator=(RealMockVendor&&) = default;
    RealMockVendor(const RealMockVendor&) = default;
    RealMockVendor& operator=(const RealMockVendor&) = default;
};

// Wrapper around RealMockVendor to satisfy AnyVendor concept
class MockVendorConceptSatisfier {
public:
    // Constructors/Assignment for concept satisfaction
    MockVendorConceptSatisfier() : real_mock_vendor_(std::make_shared<RealMockVendor>()) {}
    explicit MockVendorConceptSatisfier(std::shared_ptr<RealMockVendor> vendor) : real_mock_vendor_(std::move(vendor)) {}
    MockVendorConceptSatisfier(const MockVendorConceptSatisfier& other) = default;
    MockVendorConceptSatisfier& operator=(const MockVendorConceptSatisfier& other) = default;
    MockVendorConceptSatisfier(MockVendorConceptSatisfier&& other) noexcept = default;
    MockVendorConceptSatisfier& operator=(MockVendorConceptSatisfier&& other) noexcept = default;

    void* Alloc(size_t size, align_t align) {
        return real_mock_vendor_->MockAlloc(size, align);
    }
    void Dealloc(void* ptr, size_t size, align_t align) {
        return real_mock_vendor_->MockDealloc(ptr, size, align);
    }

    // Friend declarations for non-member equality operators for concept satisfaction
    friend bool operator==(const MockVendorConceptSatisfier& lhs, const MockVendorConceptSatisfier& rhs);
    friend bool operator!=(const MockVendorConceptSatisfier& lhs, const MockVendorConceptSatisfier& rhs);

    RealMockVendor& get_real_mock() { return *real_mock_vendor_; }

private:
    std::shared_ptr<RealMockVendor> real_mock_vendor_;
};

// Non-member equality operators implementations for MockVendorConceptSatisfier
inline bool operator==(const MockVendorConceptSatisfier& lhs, const MockVendorConceptSatisfier& rhs) {
    return lhs.real_mock_vendor_ == rhs.real_mock_vendor_;
}
inline bool operator!=(const MockVendorConceptSatisfier& lhs, const MockVendorConceptSatisfier& rhs) {
    return !(lhs == rhs);
}

// Real mock object that holds MOCK_METHODs for AnyPool's base functionality
class RealMockPool {
public:
    // MOCK_METHODS for ContinuousAlloc/Dealloc
    MOCK_METHOD(void*, MockContinuousAlloc, (size_t n));
    MOCK_METHOD(void, MockContinuousDealloc, (void* ptr, size_t n));

    // MOCK_METHODS for DiscreteAlloc/Dealloc
    MOCK_METHOD(void*, MockDiscreteAlloc, ()); // DiscreteAlloc takes no args
    MOCK_METHOD(void, MockDiscreteDealloc, (void* ptr)); // DiscreteDealloc takes ptr only

    // Required by concept.h (for AnyPool, even though only methods are mocked)
    constexpr static size_t chunk_size = 1024;
    size_t get_alloc_unit_size() { return 1; }
    size_t get_alloc_unit_align() { return 1; }

    // Disable copy and move (RealMockPool itself is not copyable/movable)
    RealMockPool() = default;
    RealMockPool(const RealMockPool&) = delete;
    RealMockPool& operator=(const RealMockPool&) = delete;
    RealMockPool(RealMockPool&&) = delete;
    RealMockPool& operator=(RealMockPool&&) = delete;
};

// Wrapper around RealMockPool to satisfy AnyPool concept
class MockPoolConceptSatisfier {
public:
    // AnyPool concept requirements
    static constexpr bool kInMemoryOptimization = false;

    // Constructors/Assignment for concept satisfaction
    MockPoolConceptSatisfier() : real_mock_pool_(std::make_shared<RealMockPool>()) {}
    explicit MockPoolConceptSatisfier(std::shared_ptr<RealMockPool> pool) : real_mock_pool_(std::move(pool)) {}
    MockPoolConceptSatisfier(const MockPoolConceptSatisfier& other) : real_mock_pool_(other.real_mock_pool_) {}
    MockPoolConceptSatisfier(MockPoolConceptSatisfier&& other) noexcept : real_mock_pool_(std::move(other.real_mock_pool_)) {}
    MockPoolConceptSatisfier& operator=(const MockPoolConceptSatisfier& other) {
        if (this != &other) {
            real_mock_pool_ = other.real_mock_pool_;
        }
        return *this;
    }
    MockPoolConceptSatisfier& operator=(MockPoolConceptSatisfier&& other) noexcept {
        if (this != &other) {
            real_mock_pool_ = std::move(other.real_mock_pool_);
        }
        return *this;
    }

    template <typename T>
    T* ContinuousAlloc(size_t n) {
        return reinterpret_cast<T*>(real_mock_pool_->MockContinuousAlloc(n * sizeof(T)));
    }

    template <typename T>
    void ContinuousDealloc(T* ptr, size_t n) {
        real_mock_pool_->MockContinuousDealloc(reinterpret_cast<void*>(ptr), n * sizeof(T));
    }

    template <typename T>
    T* DiscreteAlloc() {
        return reinterpret_cast<T*>(real_mock_pool_->MockDiscreteAlloc());
    }

    template <typename T>
    void DiscreteDealloc(T* ptr) {
        real_mock_pool_->MockDiscreteDealloc(reinterpret_cast<void*>(ptr));
    }

    // Dummy implementations for other AnyPool concept requirements
    template <typename T, typename... Args>
    T* New(Args&&... args) { return nullptr; }

    template <typename T>
    void Del(T* ptr) {}

    void Clear() {}

    // Required by concept.h
    constexpr static size_t chunk_size = 1024;
    size_t get_alloc_unit_size() { return 1; }
    size_t get_alloc_unit_align() { return 1; }

    // Equality operators for concept satisfaction
    bool operator==(const MockPoolConceptSatisfier& other) const {
        return real_mock_pool_ == other.real_mock_pool_;
    }
    bool operator!=(const MockPoolConceptSatisfier& other) const {
        return !(*this == other);
    }

    // Accessor for the real mock object to set expectations
    RealMockPool& get_real_mock() { return *real_mock_pool_; }

private:
    std::shared_ptr<RealMockPool> real_mock_pool_;
};

} // namespace crystal::mem

#endif