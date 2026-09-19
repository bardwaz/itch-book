#pragma once

#include <cstdint>
#include <cstring>

namespace itch {

// Helper functions for reading big-endian data from a byte buffer.
// We use std::memcpy to avoid strict aliasing violations and undefined behavior.
// __builtin_bswap functions are compiler-specific but widely supported (GCC/Clang)
// and emit highly optimized, single-instruction byte swaps where possible.

inline uint16_t read_be16(const char* buf) {
    uint16_t val;
    std::memcpy(&val, buf, sizeof(val));
    return __builtin_bswap16(val);
}

inline uint32_t read_be32(const char* buf) {
    uint32_t val;
    std::memcpy(&val, buf, sizeof(val));
    return __builtin_bswap32(val);
}

inline uint64_t read_be64(const char* buf) {
    uint64_t val;
    std::memcpy(&val, buf, sizeof(val));
    return __builtin_bswap64(val);
}

// ITCH timestamps are 6 bytes (48-bit) nanoseconds since midnight.
// We manually reconstruct them from big-endian layout into a uint64_t.
inline uint64_t read_be48(const char* buf) {
    const auto* b = reinterpret_cast<const uint8_t*>(buf);
    uint64_t val = 0;
    val |= static_cast<uint64_t>(b[0]) << 40;
    val |= static_cast<uint64_t>(b[1]) << 32;
    val |= static_cast<uint64_t>(b[2]) << 24;
    val |= static_cast<uint64_t>(b[3]) << 16;
    val |= static_cast<uint64_t>(b[4]) << 8;
    val |= static_cast<uint64_t>(b[5]);
    return val;
}

} // namespace itch
