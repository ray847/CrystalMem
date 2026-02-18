#include <array>    // std::array
#include <iostream> // std::cout

#include <CrystalMem/memory.h>

struct BigObject {
  int val;
  std::array<int, 64> arr;
};
using MediumObject = std::array<int, 4>;
using SmallObject = std::byte;

int main() {

  using std::cout, std::endl, std::array;

  crystal::mem::DebugResource<crystal::mem::OSResource, "memory"> mem_resource;
  crystal::mem::DebugResource<crystal::mem::OSResource, "logic"> logic_resource;
  crystal::mem::Vendor mem_vendor{mem_resource};
  crystal::mem::Vendor logic_vendor{logic_resource};
  //crystal::mem::
  //    SLUBPool<64, { 8, 32 }, decltype(mem_vendor), decltype(logic_vendor)>
  //        pool{ mem_vendor, logic_vendor };
  crystal::mem::
      SafeBestFitPool<512, decltype(mem_vendor), decltype(logic_vendor)>
          pool{ mem_vendor, logic_vendor };

  auto small = pool.New<SmallObject>();
  MediumObject* medium_arr = pool.ContinuousAlloc<MediumObject>(3);
  auto big = pool.New<BigObject>();

  cout << "Small Object: " << small << endl;
  cout << "Medium Object Array: " << medium_arr << endl;
  cout << "Big Object: " << big << endl;

  pool.Del(small);
  pool.ContinuousDealloc(medium_arr, 3);
  pool.Del(big);

  return 0;
}
