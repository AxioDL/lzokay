#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <array>

namespace lzokay {

enum class EResult {
  LookbehindOverrun = -4,
  OutputOverrun = -3,
  InputOverrun = -2,
  Error = -1,
  Success = 0,
  InputNotConsumed = 1,
};

class DictBase {
protected:
  static constexpr uint32_t HashSize = 0x4000;
  static constexpr uint32_t MaxDist = 0xbfff;
  static constexpr uint32_t MaxMatchLen = 0x800;
  static constexpr uint32_t BufSize = MaxDist + MaxMatchLen;

  /* List encoding of previous 3-byte data matches */
  struct Match3 {
    std::array<uint16_t, HashSize> head;     /* key -> chain-head-pos */
    std::array<uint16_t, HashSize> chain_sz; /* key -> chain-size */
    std::array<uint16_t, BufSize> chain;     /* chain-pos -> next-chain-pos */
    std::array<uint16_t, BufSize> best_len;  /* chain-pos -> best-match-length */
  };
  /* Encoding of 2-byte data matches */
  struct Match2 {
    std::array<uint16_t, 1 << 16> head; /* 2-byte-data -> head-pos */
  };

  struct Data {
    Match3 match3;
    Match2 match2;

    /* Circular buffer caching enough data to access the maximum lookback
     * distance of 48K + maximum match length of 2K. An additional 2K is
     * allocated so the start of the buffer may be replicated at the end,
     * therefore providing efficient circular access.
     */
    std::array<uint8_t, BufSize + MaxMatchLen> buffer;
  };
  using storage_type = Data;
  storage_type* _storage;
  DictBase() = default;
  friend struct State;
  friend EResult compress(const uint8_t* src, std::size_t src_size, uint8_t* dst, std::size_t& dst_size,
                          DictBase& dict);
};
template <template <typename> class _Alloc = std::allocator>
class Dict : public DictBase {
  _Alloc<DictBase::storage_type> _allocator;

public:
  Dict() { _storage = _allocator.allocate(1); }
  ~Dict() { _allocator.deallocate(_storage, 1); }
};

EResult decompress(const uint8_t* src, std::size_t src_size, uint8_t* dst, std::size_t dst_size, std::size_t& out_size);
EResult compress(const uint8_t* src, std::size_t src_size, uint8_t* dst, std::size_t dst_size, std::size_t& out_size,
                 DictBase& dict);
inline EResult compress(const uint8_t* src, std::size_t src_size, uint8_t* dst, std::size_t dst_size,
                        std::size_t& out_size) {
  Dict<> dict;
  return compress(src, src_size, dst, dst_size, out_size, dict);
}

constexpr std::size_t compress_worst_size(std::size_t s) { return s + s / 16 + 64 + 3; }

} // namespace lzokay
