#ifndef CRYSTALMEM_POOL_H_
#define CRYSTALMEM_POOL_H_

#include "CrystalMem/options.h" // Characterisctics
#include "CrystalMem/pool/pool_impl.h" // PoolImpl

namespace crystal::mem {

template<typename Resource, typename Characterisctic = Characteristic<>>
using Pool = PoolImpl<kPageSize, Resource>;

} // namespace mem

#endif
