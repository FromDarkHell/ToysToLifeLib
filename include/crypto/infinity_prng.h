#pragma once
#include <cstdint>

/**
 * @brief Disney Infinity uses a slightly different implementation of the LEGO Dimensions, BURTLE PRNG.
 *
 */
class InfinityPRNG
{
public:
    InfinityPRNG() : a(0), b(0), c(0), d(0) {}

    /// @brief Seeds the PRNG
    /// @param seed The (descrambled) seed value carried by the SEED command
    void init(uint32_t seed)
    {
        a = 0xF1EA5EED;
        b = c = d = seed;
        for (int i = 0; i < 23; i++)
        {
            next();
        }
    }

    /// @brief Advances the PRNG and returns the next value, used to answer a CHALLENGE command.
    /// @return uint32_t
    uint32_t next()
    {
        uint32_t oldA = a;
        uint32_t oldB = b;
        uint32_t oldC = c;
        uint32_t ret = rotl(b, 27);

        const uint32_t temp = oldA + ((ret ^ 0xFFFFFFFFu) + 1u);
        b = oldB ^ rotl(oldC, 17);
        a = d;
        c = oldC + a;
        ret = b + temp;
        a = a + temp;

        c = a;
        a = b;
        b = oldC + d;
        d = ret;

        return ret;
    }

    /// @brief The fixed bit-permutation mask used to scramble/descramble handshake values.
    static constexpr uint64_t SCRAMBLE_MASK = 0x8E55AA1B3999E8AAULL;

    /// @brief Descrambles the 32-bit seed/value from a scrambled 64-bit handshake blob.
    /// @param scrambled The raw 8-byte (big-endian) value carried by a SEED/CHALLENGE packet
    /// @return uint32_t The descrambled value
    static uint32_t descramble(uint64_t scrambled)
    {
        uint64_t mask = SCRAMBLE_MASK;
        uint32_t ret = 0;

        for (int i = 0; i < 64; i++)
        {
            if (mask & 0x8000000000000000ULL)
            {
                ret = (ret << 1) | static_cast<uint32_t>(scrambled & 0x01);
            }

            scrambled >>= 1;
            mask <<= 1;
        }

        return ret;
    }

    /// @brief Scrambles a 32-bit value's bits with `garbage` bits, to create the scrambled 64-bit blob
    /// @param value The value to scramble (i.e. a seed, or the next PRNG output)
    /// @param garbage Filler bits interleaved into the positions `value`'s bits don't occupy
    /// @return uint64_t The scrambled value
    static uint64_t scramble(uint32_t value, uint32_t garbage)
    {
        uint64_t mask = SCRAMBLE_MASK;
        uint64_t ret = 0;

        for (int i = 0; i < 64; i++)
        {
            ret <<= 1;

            if (mask & 1)
            {
                ret |= (value & 1);
                value >>= 1;
            }
            else
            {
                ret |= (garbage & 1);
                garbage >>= 1;
            }

            mask >>= 1;
        }

        return ret;
    }

private:
    static uint32_t rotl(uint32_t x, int n) { return (x << n) | (x >> (32 - n)); }

    uint32_t a, b, c, d;
};
