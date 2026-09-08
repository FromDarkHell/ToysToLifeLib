#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * @brief A tiny SHA-1 implementation, used to get per-figure AES key Disney Infinity
 * uses to encrypt a figure's character-ID block (see infinity/figure_crypto.h).
 *
 * Mostly pulled from: https://github.com/rweather/arduinolibs/blob/master/libraries/CryptoLegacy/src/SHA1.cpp
 */
class SHA1
{
public:
    static constexpr size_t HASH_SIZE = 20;
    static constexpr size_t BLOCK_SIZE = 64;

    SHA1();

    void reset();
    void update(const void *data, size_t len);
    void finalize(uint8_t hash[HASH_SIZE]);

    /**
     * @brief Convenience one-shot helper: hashes `len` bytes of `data` in a single call.
     */
    static void hash(const void *data, size_t len, uint8_t out[HASH_SIZE]);

private:
    uint32_t _h[5];
    uint32_t _w[16];
    uint64_t _length;
    uint8_t _chunkSize;

    void processChunk();
};
