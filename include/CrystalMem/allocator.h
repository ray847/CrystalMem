#ifndef CRYSTALMEM_ALLOCATOR_H_
#define CRYSTALMEM_ALLOCATOR_H_

#include "CrystalMem/options.h" // Characteristics
#include "CrystalMem/allocator/dynamic_allocator_impl.h" // DynAllocatorImpl
#include "CrystalMem/allocator/mono_allocator_impl.h" // MonoAllocatorImpl

namespace crystal::mem {

template<typename T, typename Pool, typename Characteristic = Characteristic<>>
using MonoAllocator = MonoAllocatorImpl<T, Pool>;

template<typename T, typename Pool, typename Characteristic = Characteristic<>>
using DynAllocator = DynAllocatorImpl<T, Pool>;

template<typename T, typename Pool, typename Characteristic = Characteristic<>>
using Allocator = DynAllocatorImpl<T, Pool>;

} // namespace crystal::mem

#endif
