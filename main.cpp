#include <array> // std::array
#include "CrystalMem/vendor.h"

#include <CrystalMem/memory.h>

struct BigObject {
  int val;
  std::array<int, 64> arr;
};

using SmallObject = std::byte;

int main() {
  crystal::mem::OSResource resource;
  crystal::mem::Vendor vendor{resource};
  crystal::mem::SLUBPool<128, {8, 32}, decltype(vendor)> pool{vendor};

  auto obj1 = pool.New<SmallObject>();

  pool.Del(obj1);

  return 0;
}
