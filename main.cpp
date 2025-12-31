#include <array> // std::array
#include <filesystem> // std::filesystem::path
#include <vector> // std::vector
#include <list> // std::vector

#include <CrystalUtil/log.h>

#include <CrystalMem/memory.h>

struct BigObject {
  int val;
  std::array<int, 64> arr;
};

using SmallObject = std::byte;

crystal::util::Logger logger{"./"};

int main() {
  using namespace crystal::mem;
  auto resource = Resource<Source::OS>(&logger);
  Pool<decltype(resource)> pool(resource);

  logger.Debug("Allocating from pool.");
  auto obj0 = pool.New<BigObject>();
  obj0->val = 1;
  obj0->arr[2] = 3;
  auto obj1 = pool.New<SmallObject>();
  pool.Del(obj0);
  pool.Del(obj1);

  logger.Debug("Clearing the pool.");
  pool.Clear();

  logger.Debug("Allocating vector from resource.");
  std::vector<BigObject, DynAllocator<BigObject, decltype(pool)>> vec{
    DynAllocator<BigObject, decltype(pool)>{pool}
  };
  vec.emplace_back();
  vec.emplace_back();
  vec.emplace_back();
  vec.emplace_back();

  logger.Debug("Clearing the pool.");
  pool.Clear();

  logger.Debug("Allocating list from resource.");
  std::list<BigObject, MonoAllocator<BigObject, decltype(pool)>> lst{
    MonoAllocator<BigObject, decltype(pool)>{pool}
  };
  lst.emplace_back();
  lst.emplace_back();
  lst.emplace_back();
  lst.emplace_back();

  return 0;
}
