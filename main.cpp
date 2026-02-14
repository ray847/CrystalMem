#include <array> // std::array
#include <iostream> // std::cout

#include <CrystalMem/memory.h>

struct BigObject {
  int val;
  std::array<int, 64> arr;
};
using MediumObject = std::array<int, 4>;
using SmallObject = std::byte;

int main() {

  using std::cout, std::endl;

  crystal::mem::DebugResource<crystal::mem::OSResource> resource;
  crystal::mem::Vendor vendor{resource};
  crystal::mem::SLUBPool<64, {8, 32}, decltype(vendor)> pool{vendor};

  auto small = pool.New<SmallObject>();
  auto medium1 = pool.New<MediumObject>();
  auto medium2 = pool.New<MediumObject>();
  auto big = pool.New<BigObject>();

  cout << "Small Object: " << small << endl;
  cout << "Medium Object1: " << medium1 << endl;
  cout << "Medium Object2: " << medium2 << endl;
  cout << "Big Object: " << big << endl;

  pool.Del(small);
  pool.Del(medium1);
  pool.Del(medium2);
  pool.Del(big);

  return 0;
}
