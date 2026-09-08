#pragma once

#include <stddef.h>
#include <stdint.h>

namespace DisneyInfinity
{
    /**
     * @brief Encodes/decodes a Disney Infinity figure's encrypted character-ID block.
     *
     * Real figures store their numeric character ID at game-side block 0 (the storage block
     * `Figure::toStorageBlock(0)` maps to), AES-128-ECB encrypted with a key derived from the
     * figure's own 7-byte UID via SHA-1. This whole pipeline - key derivation, plaintext block
     * layout, and the CRC32 variant used inside it - has been verified against both CEMU's
     * nsyshid/Infinity.cpp (which real games work against) and a real PS3 + Infinity Base USB
     * capture (decrypting a genuine figure's block 0 correctly recovered its real character ID).
     */
    class FigureCrypto
    {
    public:
        static const uint8_t BLOCK_SIZE = 16;
        static const uint8_t KEY_SIZE = 16;
        static const uint8_t UID_SIZE = 7;

        /**
         * @brief Derives the per-figure AES-128 key from a figure's 7-byte UID: SHA-1 of a fixed
         * 31-byte constant followed by the UID, then the first 16 digest bytes with each 4-byte
         * group byte-reversed.
         */
        static void deriveKey(const uint8_t uid[UID_SIZE], uint8_t outKey[KEY_SIZE]);

        /**
         * @brief Builds the plaintext character-ID block for a given figure ID, matching CEMU's
         * `GenerateBlankFigureData` layout: `[0x00][3-byte big-endian figureId][0x00 x5][0x01]
         * [0xD1][0x1F][4-byte big-endian CRC32 of bytes 0-11]`.
         */
        static void buildPlaintextBlock(uint32_t figureId, uint8_t outBlock[BLOCK_SIZE]);

        /**
         * @brief Encrypts a plaintext character-ID block, ready to store at game-side block 0.
         */
        static void encryptBlock(const uint8_t key[KEY_SIZE], const uint8_t plaintext[BLOCK_SIZE],
                                  uint8_t outCiphertext[BLOCK_SIZE]);

        /**
         * @brief Decrypts a figure's game-side block 0 back to its plaintext character-ID block.
         */
        static void decryptBlock(const uint8_t key[KEY_SIZE], const uint8_t ciphertext[BLOCK_SIZE],
                                  uint8_t outPlaintext[BLOCK_SIZE]);

        /**
         * @brief Extracts the numeric figure ID (bytes 1-3, big-endian) from a decrypted
         * plaintext character-ID block.
         */
        static uint32_t extractFigureId(const uint8_t plaintext[BLOCK_SIZE]);

        /**
         * @brief The specific CRC32 variant Infinity uses for the character-ID block's checksum:
         * the standard reflected table (polynomial 0xEDB88320), but with a zero initial value and
         * no final XOR (unlike zlib/PKZIP's CRC-32, which uses 0xFFFFFFFF for both).
         */
        static uint32_t crc32(const uint8_t* data, size_t len);
    };
}
