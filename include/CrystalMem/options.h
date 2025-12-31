#ifndef CRYSTALMEM_CHARACTERISTICS_H_
#define CRYSTALMEM_CHARACTERISTICS_H_

namespace crystal::mem {

enum class LongevityCa;
enum class SizeCa;
enum class ExpansionCa;
enum class Source;
template<
LongevityCa kLongevityCa,
SizeCa kSizeCa,
ExpansionCa kExpansionCa>
struct Characteristic;
template<Source, Characteristic>
struct Opts;

enum class LongevityCa {
  Eternal,
  Long,
  Regular,
  Tmp,
  Instant,
};

enum class SizeCa {
  _4B,
  _8B,
  _16B,
  _32B,
  _64B,
  _128B,
  _256B,
  _512B,
  _1kB,
  _2kB,
  _4kB,
  _infB,

  /* Aliases */
  Tiny = _8B,
  Small = _32B,
  Regular = _128B,
  Big = _1kB,
  Large = _4kB,
};

enum class ExpansionCa {
  x0,
  x_5,
  x2,
  x4,
  x64,
  x1024,
  xinf,

  /* Aliases */
  None = x0,
  Little = x_5,
  Regular = x4,
  Big = x1024,
};

enum class Source {
  OS,
  Dax,
  File,
  Fixed,
};

template<
LongevityCa kLongevityCa = LongevityCa::Regular,
SizeCa kSizeCa = SizeCa::Regular,
ExpansionCa kExpansionCa = ExpansionCa::Regular>
struct Characteristic{
  static constexpr LongevityCa kLongevity = kLongevityCa;
  static constexpr SizeCa kSize = kSizeCa;
  static constexpr ExpansionCa kExpansion = kExpansionCa;
  Characteristic() = delete;
  ~Characteristic() = delete;
};

} // namespace crystal::mem

#endif
